#if !defined(DEEP_SLEEP_KERNEL_H)
#define DEEP_SLEEP_KERNEL_H

#include "application.h"
#include "projects/brin-sulawesi/models/task_context.h"

class FSMKernel
{
private:
    static const bool SLEEP_MODE = false;         // Set true for sleep
    static const uint8_t MAX_TICK_PUBLISH = 6;    // How many tick until publish state
    static const uint8_t MAX_RETRIES_PUBLISH = 3; // Maximum allowed attempt retries publish MQTT

    // State holder variable
    ApplicationState _appState = ApplicationState::SAMPLING_STATE;
    BatteryState _battState = BatteryState::SAFE_STATE;
    CalibrationMode _calState = CalibrationMode::CAL_UNKNOWN;

    // Task context
    SensorContext *_sensorCtx;
    NetworkingContext *_netCtx;

    // Peripheral for small task
    uint8_t _tick = 0;
    SensorObject _sensorDatas[MAX_TICK_PUBLISH];
    bool _mqttPublishTimeout = false;

public:
    FSMKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx) {}
    ~FSMKernel() {}

    void loop()
    {
        // Battery State Driver
        const float batteryVoltage = _sensorCtx->accuINA.readBusVoltage();
        if (batteryVoltage < GlobalConfig::BATTERY_DANGER)
        {
            _battState = BatteryState::DANGER_STATE;
            _appState = ApplicationState::SUSPEND_STATE;
        }
        else if (batteryVoltage < GlobalConfig::BATTERY_WARNING)
        {
            _battState = BatteryState::WARNING_STATE;
            _appState = ApplicationState::SAMPLING_STATE;
        }
        else
        {
            _battState = BatteryState::SAFE_STATE;
            _appState = ApplicationState::SAMPLING_STATE;
        }

        // Main Application State Driver
        if (_tick < MAX_TICK_PUBLISH && _appState != ApplicationState::SUSPEND_STATE)
            _appState = ApplicationState::SAMPLING_STATE;

        else if (_tick >= MAX_TICK_PUBLISH && _appState != ApplicationState::SUSPEND_STATE)
            _appState = ApplicationState::PUBLISH_STATE;

        // Sub-kernel driver (Blocking)
        while (_calState != CAL_UNKNOWN)
        {
            _netCtx->mqtt.poll();

            static uint32_t prevPublish = millis();
            if (millis() - prevPublish > 3000)
            {
                prevPublish = millis();
                if (_calState == CAL_ALL)
                {
                    SensorObject sensor = Application::samplingTask(_sensorCtx);
                    _netCtx->mqtt.publish(GlobalConfig::SENSOR_STREAM_TOPIC, sensor.toCSV());
                }
                else if (_calState == CAL_TURBIDITY)
                {
                    SensorObject sensor = Application::samplingTask(_sensorCtx);
                    _netCtx->mqtt.publish(GlobalConfig::SENSOR_STREAM_TOPIC, sensor.toCSV());
                }
            }
        }

        // === State Task Declaration ===
        if (_appState == ApplicationState::SAMPLING_STATE)
        {
            _netCtx->wifi.disableWiFi();

            SensorObject sensorData = Application::samplingTask(_sensorCtx);
            _sensorDatas[_tick++] = sensorData;

            if (SLEEP_MODE)
                esp_light_sleep_start();
            else
                delay(10000);
        }
        else if (_appState == ApplicationState::PUBLISH_STATE)
        {
            SensorObject sensorAverage{};
            for (uint8_t i = 0; i < _tick; i++)
            {
                (sensorAverage.turbidity += _sensorDatas[i].turbidity) /= 2;
                (sensorAverage.awlr += _sensorDatas[i].awlr) /= 2;
                (sensorAverage.battery += _sensorDatas[i].battery) /= 2;
            }

            const uint32_t attemptedTimestamp = millis();
            while (millis() - attemptedTimestamp < 15000)
            {
                if (!Application::connectToWiFiTask(_netCtx))
                    esp_restart(); // Assume hang connecting to WiFi

                if (Application::connectToMqttTask(
                        _netCtx,
                        [this](CalibrationMode cMode)
                        {
                            _calState = cMode;
                        }))
                {
                    SensorLogAggregate agregateData(sensorAverage);
                    uint32_t retryTimestamp = millis();
                    bool isPublishSucccess = Application::publishMqttTask(
                        _netCtx,
                        agregateData.toCSV(_netCtx->wifi.getRssi()));
                    while (!isPublishSucccess &&
                           (millis() - retryTimestamp > 5000)) // Timeout in 5 second
                        delay(500);                            // Backoff retrying
                    _mqttPublishTimeout = false;
                }
            }
            _mqttPublishTimeout = true;
            _tick = 0;
        }
    }
};

#endif // DEEP_SLEEP_KERNEL_H
