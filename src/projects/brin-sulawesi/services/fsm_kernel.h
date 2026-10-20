#if !defined(DEEP_SLEEP_KERNEL_H)
#define DEEP_SLEEP_KERNEL_H

#include "application.h"
#include "projects/brin-sulawesi/models/task_context.h"
#include "projects/brin-sulawesi/utils/enum.h"

class FSMKernel
{
private:
    const bool IS_MILLIS = true;

    ApplicationState _state = ApplicationState::SAMPLING_STATE;
    SensorContext *_sensorCtx;
    NetworkingContext *_netCtx;

    uint8_t _cycle = 0;

public:
    FSMKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx) {}
    ~FSMKernel() {}

    void loop()
    {
        /**
         * @brief Automated Cycle FSM
         * 1 Cycle is 30 second
         * adjust to usecase how long a state should last
         */
        if (_cycle < 3)
            _state = ApplicationState::SAMPLING_STATE;

        if (_cycle >= 3)
            _state = ApplicationState::PUBLISH_STATE;

        if (_state == ApplicationState::SAMPLING_STATE)
        {
            _netCtx->wifi.disableWiFi();

            SensorObject sensorData = Application::samplingTask(_sensorCtx);
            Application::sensorAgregatorTask(sensorData, false);
            Serial.printf("[INFO] Sampling: %s\n", sensorData.toString());
            Preferences prefs;
            prefs.begin("app_config", true);
            const String currentVersion = prefs.getString("version", "");
            Serial.printf("Current ver: %s\n", currentVersion);
            _cycle++;

            if (!IS_MILLIS)
                esp_light_sleep_start();
            else
                delay(10000);
        }
        else if (_state == ApplicationState::PUBLISH_STATE)
        {
            SensorObject sensorData = Application::samplingTask(_sensorCtx);
            SensorObject snapshot = Application::sensorAgregatorTask(sensorData, true);
            MqttPayload payload = MqttPayload{
                .topic = GlobalConfig::SENSOR_TOPIC,
                .message = snapshot.toCSV()};

            const uint32_t attemptedTimestamp = millis();
            bool isPublishSuccess = false;
            while (!isPublishSuccess && millis() - attemptedTimestamp < 15000)
            {
                isPublishSuccess = Application::networkingTask(_netCtx, payload);
                if (isPublishSuccess)
                {
                    Application::sensorAgregatorTask({0.0f, 0.0f, 0.0f}, true);
                    _cycle = 0;
                }
                delay(1000);
            }

            if (!isPublishSuccess)
            {
                Serial.println("[ERROR] Publish mqtt timed out");
                _cycle = 0;
            }
        }
    }
};

#endif // DEEP_SLEEP_KERNEL_H
