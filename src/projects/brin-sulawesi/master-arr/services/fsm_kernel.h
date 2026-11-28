#if !defined(DEEP_SLEEP_KERNEL_H)
#define DEEP_SLEEP_KERNEL_H

#include "application.h"
#include "db.h"
#include "../models/task_context.h"

class FSMKernel
{
private:
    static const bool SLEEP_MODE = false;                                // Set true for sleep
    static const uint32_t DELAY_SAMPLING = GlobalConfig::DELAY_SAMPLING; // in ms
    static const uint32_t MAX_INTERVAL_PUBLISH = 30UL * 60UL * 1000UL;   // 30 minutes, in ms
    static const uint32_t TIMEOUT_PUBLISH = 15000;                       // in ms
    static const uint32_t RETRY_BACKOFF_PUBLISH = 500;                   // in ms

    // State holder variable
    ApplicationState _appState = ApplicationState::SAMPLING_STATE;
    BatteryState _battState = BatteryState::SAFE_STATE;
    CalibrationMode _calState = CalibrationMode::CAL_UNKNOWN;

    // Task context
    SensorContext *_sensorCtx;
    NetworkingContext *_netCtx;

    // Publish interval & sample accumulation
    uint32_t _intervalPublish = GlobalConfig::DELAY_TX; // in ms, loaded from DB
    uint16_t _samplesPerPublish = 1;                    // derived from _intervalPublish
    uint16_t _sampleCount = 0;                          // samples accumulated since last publish
    SensorObject _sampleSum{};                          // running sum, averaged on publish
    bool _mqttPublishTimeout = false;

    // Reload interval from DB and derive how many samples make up one publish window
    void loadInterval()
    {
        uint32_t interval = DB::getInterval(GlobalConfig::DELAY_TX);
        if (interval > MAX_INTERVAL_PUBLISH)
            interval = MAX_INTERVAL_PUBLISH;
        _intervalPublish = interval;

        const uint32_t samples = interval / DELAY_SAMPLING;
        _samplesPerPublish = samples < 1 ? 1 : samples;
        Serial.printf("[INFO] Publish interval: %lu ms (%u samples)\n",
                      (unsigned long)_intervalPublish, _samplesPerPublish);
    }

    void resetSamples()
    {
        _sampleSum = SensorObject{};
        _sampleCount = 0;
    }

public:
    FSMKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx)
    {
        loadInterval();
    }
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
        const bool isIntervalTurn = _sampleCount >= _samplesPerPublish;
        if (_appState != ApplicationState::SUSPEND_STATE &&
            !isIntervalTurn)
        {
            Serial.println("[INFO] ON Sampling State");
            _appState = ApplicationState::SAMPLING_STATE;
        }
        else if (_appState != ApplicationState::SUSPEND_STATE &&
                 isIntervalTurn)
        {
            Serial.println("[INFO] ON Publish State");
            _appState = ApplicationState::PUBLISH_STATE;
        }

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
            _netCtx->mqtt.close();
            _netCtx->wifi.disableWiFi();

            const SensorObject sensorData = Application::samplingTask(_sensorCtx);
            _sampleSum.tipping = sensorData.tipping; // daily count from the slave: keep latest, don't average
            _sampleSum.voltAccu += sensorData.voltAccu;
            _sampleSum.voltPanel += sensorData.voltPanel;
            _sampleCount++;

            if (SLEEP_MODE)
                esp_light_sleep_start();
            else
                delay(DELAY_SAMPLING);
        }
        else if (_appState == ApplicationState::PUBLISH_STATE)
        {
            const float count = _sampleCount > 0 ? float(_sampleCount) : 1.0f;
            const SensorObject averagedSensor = SensorObject{
                .tipping = _sampleSum.tipping, // latest sample, already a running daily count
                .voltAccu = _sampleSum.voltAccu / count,
                .voltPanel = _sampleSum.voltPanel / count,
            };

            bool isPublishSuccess = false;
            const uint32_t attemptedTimestamp = millis();
            while (!isPublishSuccess &&
                   millis() - attemptedTimestamp < TIMEOUT_PUBLISH)
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
                    SensorLogAggregate agregateData(averagedSensor);
                    isPublishSuccess = Application::publishMqttTask(
                        _netCtx,
                        agregateData.toCSV(_netCtx->wifi.getRssi()));
                }

                if (!isPublishSuccess)
                    delay(RETRY_BACKOFF_PUBLISH); // Backoff before retrying
            }
            _mqttPublishTimeout = !isPublishSuccess;
            if (_mqttPublishTimeout)
                Serial.println("[ERROR] Publish timed out, data for this window dropped");

            Application::syncSlaveClockTask(_sensorCtx);

            loadInterval();
            resetSamples();
        }
    }
};

#endif // DEEP_SLEEP_KERNEL_H
