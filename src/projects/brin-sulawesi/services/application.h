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
        bool isRetainReceived = false;
        static char otaVersion[32];
        if (isMQTTConnected)
        {
            bool success = mqtt.publish(payload.topic, payload.message);
            if (!success)
            {
                Serial.println("[INFO] Failed to publish /data");
                return false;
            }
            Serial.println("[INFO] Success publish /data");

            static char otaUrl[512];
            mqtt.subscribe(
                GlobalConfig::OTA_TOPIC,
                [&isRetainReceived](const char *msg, const size_t len)
                {
                    JsonDocument doc;
                    if (deserializeJson(doc, msg, len))
                        return; // malformed JSON
                    const char *url = doc["url"];
                    const char *version = doc["version"];
                    if (!url || !version || strlen(url) >= sizeof(otaUrl) ||
                        strlen(version) >= sizeof(otaVersion))
                        return; // missing or too long for the buffer
                    strlcpy(otaUrl, url, sizeof(otaUrl));
                    strlcpy(otaVersion, version, sizeof(otaVersion));
                    isRetainReceived = true;
                    Serial.printf("[INFO] OTA version: %s\n", otaVersion);
                });

            uint32_t attemptSubscribed = millis();
            while (!isRetainReceived && millis() - attemptSubscribed <= 5000)
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

            if (!isRetainReceived)
            {
                Serial.println("[INFO] No OTA request received");
                return true;
            }

            Preferences prefs;
            prefs.begin("app_config", true);
            const String currentVersion = prefs.getString("version", "");
            prefs.end();
            if (currentVersion != otaVersion)
            {
                OTARequest ota(false, true);
                bool successOTA = ota.beginInsecure(otaUrl, currentVersion.c_str(), otaVersion);
                if (successOTA)
                {
                    bool successsACKOTA = mqtt.publish(GlobalConfig::OTA_RESPONSE_TOPIC, "Success");
                    Serial.printf("[INFO] ACK OTA: %d\n", successsACKOTA);
                    delay(1000);
                    esp_restart();
                }
            }
            Serial.println("[INFO] No update received");
            return true; // Since still success publish
        }
        Serial.println("[INFO] MQTT Failed to Connect");
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
