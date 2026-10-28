#if !defined(CALIBRATION_H)
#define CALIBRATION_H

#include <projects/brin-sulawesi/models/task_context.h>
#include <projects/brin-sulawesi/services/application.h>

class CalibrationKernel
{
private:
    SensorContext *_sensorCtx;
    NetworkingContext *_netCtx;

    bool sessionActive = false;

public:
    CalibrationKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx) {}

    bool setupEnvironment()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("[ERROR] WiFi is not connected for calibration");
            return false;
        }
        Serial.printf("CAL WIFI: %s\n", _netCtx->wifi.localIP());

        const bool connected = _netCtx->mqtt.connect(
            GlobalConfig::USERNAME_MQTT,
            GlobalConfig::PASSWORD_MQTT);
        if (!connected)
        {
            Serial.println("[ERROR] Failed to connect MQTT for calibration");
            return false;
        }

        const bool subscribed = _netCtx->mqtt.subscribe(
            GlobalConfig::CMD_TOPIC,
            [this](const char *payload, const size_t size)
            {
                if (payload == nullptr || size == 0)
                {
                    Serial.println("[ERROR] Empty calibration command payload");
                    return;
                }

                JsonDocument doc;
                const DeserializationError parseError = deserializeJson(doc, payload, size);
                if (parseError)
                {
                    Serial.printf("[ERROR] Invalid calibration command payload: %s\n",
                                  parseError.c_str());
                    return;
                }

                JsonDocument commandDoc;
                JsonDocument *command = &doc;
                const char *uuid = doc["commandId"];
                const char *nestedPayload = doc["payload"];
                if (nestedPayload != nullptr)
                {
                    const DeserializationError commandParseError =
                        deserializeJson(commandDoc, nestedPayload);
                    if (commandParseError)
                    {
                        Serial.printf("[ERROR] Invalid nested calibration command payload: %s\n",
                                      commandParseError.c_str());
                        return;
                    }
                    command = &commandDoc;
                }

                const char *type = (*command)["type"];
                if (type == nullptr)
                {
                    Serial.println("[ERROR] Calibration command payload requires a type");
                    return;
                }

                if (strcmp(type, "done_calibrate") == 0)
                {
                    Serial.println("[INFO] MQTT CMD DONE CALIBRATE");

                    static char buffer[128];
                    JsonDocument ackDoc;
                    ackDoc["commandId"] = uuid;
                    ackDoc["success"] = true;
                    ackDoc["message"] = "Done Calibrating";
                    serializeJson(ackDoc, buffer);

                    _netCtx->mqtt.publish(GlobalConfig::CMD_RESPONSE_TOPIC, buffer);
                    sessionActive = false;
                }
            });
        if (!subscribed)
        {
            Serial.println("[ERROR] Failed to subscribe calibration command");
            return false;
        }

        return true;
    }

    void calibrateTurbidity()
    {
        sessionActive = true;
        while (sessionActive)
        {
            _netCtx->mqtt.poll();
            SensorObject sensor = Application::samplingTask(_sensorCtx);
            _netCtx->mqtt.publish(GlobalConfig::SENSOR_STREAM_TOPIC, sensor.toCSV());
            delay(3000);
        }
    }
};

#endif // CALIBRATION_H
