#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ArduinoJson.h>
#include <Preferences.h>
#include "../consts/sensors.h"
#include "../models/task_context.h"
#include "../models/datastore.h"
#include "ota/http_ota.h"

struct MqttPayload
{
    const char *topic;
    const char *message;
};

class Application
{
public:
    static bool networkingTask(void *pvParam, MqttPayload payload)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);
        while (WiFi.status() != WL_CONNECTED)
            ctx->wifi.begin(false); // BLOCKING TIMEOUT 20S

        MQTTWebsocketSecure mqtt(GlobalConfig::BROKER_MQTT, GlobalConfig::PORT_MQTT);
        bool isMQTTConnected = mqtt.connect(GlobalConfig::USERNAME_MQTT, GlobalConfig::PASSWORD_MQTT); // BLOCKING TIMEOUT 20S
        if (isMQTTConnected)
        {
            bool successPublishData = mqtt.publish(payload.topic, payload.message);
            if (!successPublishData)
            {
                Serial.println("[ERROR] Failed to publish /data");
                return false;
            }
            Serial.println("[INFO] Success publish /data");

            bool successSubscribeOTA = mqtt.subscribe(
                GlobalConfig::OTA_TOPIC,
                [&mqtt](const char *msg, const size_t len)
                {
                    bool successUpdateVersion = _mqttOtaCallback(msg, len);
                    if (!successUpdateVersion)
                        Serial.println("[ERROR] Failed update version over OTA");
                    else
                    {
                        mqtt.publish(GlobalConfig::OTA_RESPONSE_TOPIC, "SUCCESS");
                        esp_restart();
                    }
                });

            uint32_t attemptSubscribed = millis();
            while (millis() - attemptSubscribed <= 5000)
            {
                if (!mqtt.isConnected())
                {
                    static unsigned long lastReconnect = 0;
                    if (millis() - lastReconnect > 5000)
                    {
                        Serial.println("[INFO] Reconnecting MQTT...");
                        lastReconnect = millis();
                        mqtt.connect(GlobalConfig::USERNAME_MQTT, GlobalConfig::PASSWORD_MQTT);
                    }
                }
                mqtt.poll();
                delay(1);
            }
            Serial.println("No retain message about OTA");
            return true;
        }
        Serial.println("[INFO] MQTT Failed to Connect");
        return false;
    }

    static bool _mqttOtaCallback(const char *msg, const size_t len)
    {
        JsonDocument doc;
        deserializeJson(doc, msg, len);
        const char *url = doc["url"];
        const char *version = doc["version"];

        Preferences prefs;
        prefs.begin("app_config", true);
        const String currentVersion = prefs.getString("version", "");
        prefs.end();

        if (currentVersion != version)
        {
            Serial.printf("[INFO] Update available, newest version: %s\n", version);
            OTARequest ota(false, true);
            bool successOTA = ota.beginInsecure(url, currentVersion.c_str(), version);
            if (successOTA)
                return true;
        }
        Serial.printf("[INFO] Already on newest version: %s\n", currentVersion);
        return false;
    }

    static SensorObject samplingTask(void *pvParam)
    {
        SensorContext *ctx = static_cast<SensorContext *>(pvParam);

        // ReadResult resTurbidity = ctx->turbidity.rawRead();
        // ReadResult resAwlr = ctx->awlr.rawRead();
        // float batteryRead = analogRead(A10);

        // float turbidity, waterLevel;
        // if (resTurbidity.isOk())
        //     turbidity = resTurbidity.value;
        // if (resAwlr.isOk())
        //     waterLevel = resAwlr.value;

        // Wire.begin(PIN_SDA, PIN_SCL);
        // accuINA.begin();
        // solarINA.begin();

        static SensorObject sensor = SensorObject{
            .turbidity = 10,
            .awlr = 22,
            .battery = 2.3};
        return sensor;
    }
};

#endif // APPLICATION_H
