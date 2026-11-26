#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ArduinoJson.h>
#include <Preferences.h>
#include "../consts/sensors.h"
#include "../models/task_context.h"
#include "../models/datastore.h"
#include "ota/http_ota.h"
#include "mqtt_cmd_parser.h"

class Application
{
public:
    static bool connectToWiFiTask(void *pvParam)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);
        while (WiFi.status() != WL_CONNECTED)
            if (!ctx->wifi.begin(false))
            {
                Serial.println("[ERROR] Timeout 20s triggered, failed to connect WiFi!");
                return false;
            }
        Serial.println("[INFO] Connected to WiFi");
        return true;
    }

    static bool connectToMqttTask(
        void *pvParam,
        std::function<void(CalibrationMode)> calMode)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);

        bool isMQTTConnected = ctx->mqtt.connect(
            GlobalConfig::USERNAME_MQTT, GlobalConfig::PASSWORD_MQTT); // BLOCKING TIMEOUT 20S
        if (!isMQTTConnected)
        {
            Serial.println("[INFO] MQTT Failed to Connect");
            return false;
        }
        Serial.println("[INFO] Connected to MQTT Broker");

        ctx->mqtt.subscribe(
            GlobalConfig::OTA_TOPIC,
            [ctx](const char *msg, const size_t len)
            {
                bool success = MQTTCommandParser::otaCallback(msg, len);
                if (success)
                {
                    ctx->mqtt.publish(GlobalConfig::OTA_RESPONSE_TOPIC, "SUCCESS");
                    delay(1000); // Without this publish won't get sent
                    esp_restart();
                }
            });
        ctx->mqtt.subscribe(
            GlobalConfig::PING_TOPIC,
            [ctx](const char *msg, const size_t len)
            {
                const char *res = MQTTCommandParser::pingCallback(msg, len);
                ctx->mqtt.publish(GlobalConfig::PING_RESPONSE_TOPIC, res);
            });
        ctx->mqtt.subscribe(
            GlobalConfig::CMD_TOPIC,
            [ctx, calMode](const char *msg, const size_t len)
            {
                const char *res = MQTTCommandParser::cmdCallback(msg, len, calMode);
                ctx->mqtt.publish(GlobalConfig::CMD_RESPONSE_TOPIC, res);
            });

        uint32_t attemptSubscribed = millis();
        while (millis() - attemptSubscribed <= 5000)
        {
            if (!ctx->mqtt.isConnected())
            {
                static unsigned long lastReconnect = 0;
                if (millis() - lastReconnect > 5000)
                {
                    Serial.println("[INFO] Reconnecting MQTT...");
                    lastReconnect = millis();
                    ctx->mqtt.connect(GlobalConfig::USERNAME_MQTT, GlobalConfig::PASSWORD_MQTT);
                }
            }
            ctx->mqtt.poll();
        }
        return true;
    }

    static const bool publishMqttTask(void *pvParam, const char *msg)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);

        bool successPublishData = ctx->mqtt.publish(GlobalConfig::SENSOR_TOPIC, msg);
        if (!successPublishData)
        {
            Serial.printf("[ERROR] Failed to publish /data:\n%s\n", msg);
            return false;
        }
        Serial.printf("[INFO] Success publish /data:\n%s\n", msg);
        return true;
    }

    static SensorObject samplingTask(void *pvParam)
    {
        SensorContext *ctx = static_cast<SensorContext *>(pvParam);

        ctx->turbidity.begin();
        ReadResult resTurbidity = ctx->turbidity.rawRead();
        ctx->awlr.begin();
        ReadResult resAWLR = ctx->awlr.rawRead();

        bool statusI2C = Wire.begin(ctx->PIN_SDA, ctx->PIN_SCL);
        bool statusAccu = false, statusSolar = false;
        if (statusI2C)
        {
            statusAccu = ctx->accuINA.begin();
            statusSolar = ctx->solarINA.begin();
        }

        SensorObject sensor = SensorObject{
            .turbidity = static_cast<float>(resTurbidity.isOk() ? resTurbidity.value : -resTurbidity.error),
            .awlr = static_cast<float>(resAWLR.isOk() ? resAWLR.value : -resAWLR.error),
            .voltAccu = statusAccu ? ctx->accuINA.readBusVoltage() : -0xE2,
            .voltPanel = statusSolar ? ctx->solarINA.readBusVoltage() : -0xE2};
        Serial.printf("[INFO] Sensor Data: %s\n", sensor.toString());
        return sensor;
    }
};

#endif // APPLICATION_H
