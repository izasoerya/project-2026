#if !defined(MQTT_CMD_PARSER_H)
#define MQTT_CMD_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "transmitter/configs/mqtt_module.h"
#include "projects/brin-sulawesi/consts/global_config.h"
#include <Preferences.h>
#include <ota/http_ota.h>

class MQTTCommandParser
{
private:
public:
    MQTTCommandParser() {}
    ~MQTTCommandParser() {}

    static const char *pingCallback(const char *msg, const size_t len)
    {
        JsonDocument doc;
        deserializeJson(doc, msg, len);
        const char *pingId = doc["pingId"];

        static char response[64];
        snprintf(response, sizeof(response), "%s", pingId);
        return response;
    }

    static bool otaCallback(const char *msg, const size_t len)
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
            Serial.println("[INFO] Failed to update");
            return false;
        }
        Serial.printf("[INFO] Already on newest version: %s\n", currentVersion);
        return false;
    }

    static const char *cmdCallback(const char *msg, const size_t len)
    {
        if (msg == nullptr || len == 0)
        {
            Serial.println("[ERROR] Empty command payload");
            return "";
        }

        JsonDocument doc;
        const DeserializationError parseError = deserializeJson(doc, msg, len);
        if (parseError)
        {
            Serial.printf("[ERROR] Invalid command payload: %s\n", parseError.c_str());
            return "";
        }

        JsonDocument commandDoc;
        JsonDocument *command = &doc;
        const char *uuid = doc["commandId"];
        const char *payload = doc["payload"];
        if (payload != nullptr)
        {
            const DeserializationError commandParseError = deserializeJson(commandDoc, payload);
            if (commandParseError)
            {
                Serial.printf("[ERROR] Invalid nested command payload: %s\n",
                              commandParseError.c_str());
                return "";
            }
            command = &commandDoc;
        }

        const char *type = (*command)["type"];
        if (type == nullptr)
        {
            Serial.println("[ERROR] Command payload requires a type");
            return "";
        }

        JsonDocument ackDoc;
        static char response[128];
        if (strcmp(type, "reset") == 0)
        {
            Serial.println("[INFO] MQTT CMD RESET");

            ackDoc["commandId"] = uuid;
            ackDoc["success"] = true;
            ackDoc["message"] = "Restarting Now";
            serializeJson(ackDoc, response);

            return response;
        }
        else if (strcmp(type, "get_status") == 0)
        {
            Serial.print("[INFO] MQTT CMD GET STATUS");

            ackDoc["commandId"] = uuid;
            ackDoc["success"] = true;
            ackDoc["message"] = "FINE";
            serializeJson(ackDoc, response);

            return response;
        }
        else if (strcmp(type, "calibrate") == 0)
        {
            Serial.println("[INFO] MQTT CMD CALIBRATE");
            const char *sensorId = doc["sensorId"];
            if (sensorId == nullptr)
            {
                Serial.println("[ERROR] Calibrate command requires a sensor ID");
                return "";
            }

            if (strcmp(sensorId, "9e7ad877-9412-4b47-addb-f16d507ba262") == 0)
            {
                Serial.print("[INFO] MQTT CMD CALIBRATE TURBIDITY");

                ackDoc["commandId"] = uuid;
                ackDoc["success"] = true;
                ackDoc["message"] = "START CALIBRATE TURBIDITY";
                serializeJson(ackDoc, response);

                return response;
            }
            else if (strcmp(sensorId, "tinggi_muka_air") == 0)
            {
                Serial.print("[INFO] MQTT CMD CALIBRATE TINGGI MUKA AIR");

                ackDoc["commandId"] = uuid;
                ackDoc["success"] = true;
                ackDoc["message"] = "START CALIBRATE TINGGI MUKA AIR";
                serializeJson(ackDoc, response);

                return response;
            }
            else
            {
                Serial.printf("[ERROR] Unknown sensor ID: %s\n", sensorId);
                return "";
            }
        }
        else
        {
            Serial.printf("[ERROR] Unknown command type: %s\n", type);
            return "";
        }
    }
};

#endif // MQTT_CMD_PARSER_H
