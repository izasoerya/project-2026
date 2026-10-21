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

    uint8_t _sampleCount = 0;
    const uint8_t _sampleTarget = 3;
    SensorObject _sensorDataList[8];

public:
    FSMKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx) {}
    ~FSMKernel() {}

    void loop()
    {
        // === State Driver ===
        if (_sampleCount < _sampleTarget)
            _state = ApplicationState::SAMPLING_STATE;
        else
            _state = ApplicationState::PUBLISH_STATE;

        // === State Task Declaration ===
        if (_state == ApplicationState::SAMPLING_STATE)
        {
            _netCtx->wifi.disableWiFi();

            SensorObject sensorData = Application::samplingTask(_sensorCtx);
            _sensorDataList[_sampleCount] = sensorData;
            Serial.printf("[INFO] Sampling: %s\n", sensorData.toString());

            if (!IS_MILLIS)
                esp_light_sleep_start();
            else
                delay(10000);

            _sampleCount++;
        }
        else if (_state == ApplicationState::PUBLISH_STATE)
        {
            SensorObject sensorAverage{};
            for (uint8_t i = 0; i < _sampleCount; i++)
            {
                (sensorAverage.turbidity += _sensorDataList[i].turbidity) /= 2;
                (sensorAverage.awlr += _sensorDataList[i].awlr) /= 2;
                (sensorAverage.battery += _sensorDataList[i].battery) /= 2;
            }
            MqttPayload payload = MqttPayload{
                .topic = GlobalConfig::SENSOR_TOPIC,
                .message = sensorAverage.toCSV()};

            const uint32_t attemptedTimestamp = millis();
            bool isPublishSuccess = false;
            while (!isPublishSuccess && millis() - attemptedTimestamp < 15000)
            {
                isPublishSuccess = Application::networkingTask(_netCtx, payload);
                if (isPublishSuccess)
                    _sampleCount = 0;

                delay(1000);
            }

            if (!isPublishSuccess)
            {
                Serial.println("[ERROR] Publish mqtt timed out");
                _sampleCount = 0;
            }
        }

        static uint32_t attemptLog = millis();
        if (millis() - attemptLog > 3000)
        {
            attemptLog = millis();

            Preferences prefs;
            prefs.begin("app_config", true);
            const String currentVersion = prefs.getString("version", "");
            Serial.printf("Current ver: %s\n", currentVersion);

            // if (_sensorCtx->accuINA.readBusVoltage() < GlobalConfig::BATTERY_DANGER)
            //     Serial.println("On a danger position");
            // else if (_sensorCtx->accuINA.readBusVoltage() > GlobalConfig::BATTERY_DANGER &&
            //          _sensorCtx->accuINA.readBusVoltage() < GlobalConfig::BATTERY_WARNING)
            //     Serial.println("On a warning position");
        }
    }
};

#endif // DEEP_SLEEP_KERNEL_H
