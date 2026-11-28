#if !defined(MQTT_CMD_PARSER_H)
#define MQTT_CMD_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "transmitter/configs/mqtt_module.h"
#include "../consts/global_config.h"
#include <ota/http_ota.h>
#include "db.h"
#include "slave_client.h"

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
        if (msg == nullptr || len == 0)
        {
            Serial.println("[ERROR] Empty command payload");
            return false;
        }

        JsonDocument doc;
        const DeserializationError parseError = deserializeJson(doc, msg, len);
        if (parseError)
        {
            Serial.printf("[ERROR] Invalid command payload: %s\n", parseError.c_str());
            return false;
        }
        const char *url = doc["url"];
        const char *version = doc["version"];
        if (url == nullptr || version == nullptr)
        {
            Serial.println("[ERROR] OTA payload requires url and version");
            return false;
        }

        const String currentVersion = DB::getVersion();
        if (currentVersion == version)
        {
            Serial.printf("[INFO] Already on newest version: %s\n", currentVersion.c_str());
            return false;
        }

        Serial.printf("[INFO] Update available, newest version: %s\n", version);
        OTARequest ota(false, true);
        bool successOTA = ota.beginInsecure(url, currentVersion.c_str(), version);
        if (successOTA)
        {
            DB::saveVersion(version);
            return true;
        }
        Serial.println("[INFO] Failed to update");
        return false;
    }

    static const char *cmdCallback(const char *msg, const size_t len, std::function<void(CalibrationMode)> cb)
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

        JsonDocument ackDoc;
        static char response[128];

        // Interval command has no "type": payload is {"set_interval": <ms>}
        JsonVariant intervalField = (*command)["set_interval"];
        if (!intervalField.isNull())
        {
            ackDoc["commandId"] = uuid;
            if (!intervalField.is<uint32_t>() || intervalField.as<uint32_t>() == 0 ||
                intervalField.as<uint32_t>() > 30UL * 60UL * 1000UL) // max 30 minutes
            {
                Serial.println("[ERROR] set_interval requires 1..1800000 ms");
                ackDoc["success"] = false;
                ackDoc["message"] = "Invalid set_interval value";
                serializeJson(ackDoc, response);
                return response;
            }

            const uint32_t intervalMs = intervalField.as<uint32_t>();
            const bool saved = DB::saveInterval(intervalMs);
            Serial.printf("[INFO] MQTT CMD SET INTERVAL: %lu ms (%s)\n",
                          (unsigned long)intervalMs, saved ? "saved" : "save FAILED");

            ackDoc["success"] = saved;
            ackDoc["message"] = saved ? "Interval updated" : "Failed to save interval";
            serializeJson(ackDoc, response);
            return response;
        }

        const char *type = (*command)["type"];
        if (type == nullptr)
        {
            Serial.println("[ERROR] Command payload requires a type");
            return "";
        }

        if (strcmp(type, "reset") == 0)
        {
            Serial.println("[INFO] MQTT CMD RESET");

            ackDoc["commandId"] = uuid;
            ackDoc["success"] = true;
            ackDoc["message"] = "Restarting Now";
            serializeJson(ackDoc, response);

            return response;
        }
        else if (strcmp(type, "restart_slave") == 0)
        {
            Serial.println("[INFO] MQTT CMD RESTART SLAVE");
            const bool ok = SlaveClient::restart(DEVICE_ID); // slave id == ARR DEVICE_ID, see sensors.h

            ackDoc["commandId"] = uuid;
            ackDoc["success"] = ok;
            ackDoc["message"] = ok ? "Slave restarting" : "Slave did not respond";
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
        else if (strcmp(type, "set_profile") == 0)
        {
            Serial.println("[INFO] MQTT CMD SET PROFILE");

            ackDoc["commandId"] = uuid;
            if (!(*command)["bottom"].is<float>() || !(*command)["top"].is<float>())
            {
                Serial.println("[ERROR] set_profile requires numeric 'bottom' and 'top'");
                ackDoc["success"] = false;
                ackDoc["message"] = "Invalid profile";
                serializeJson(ackDoc, response);
                return response;
            }

            const float bottom = (*command)["bottom"];
            const float top = (*command)["top"];
            if (bottom >= top)
            {
                Serial.println("[ERROR] set_profile requires bottom < top");
                ackDoc["success"] = false;
                ackDoc["message"] = "bottom must be lower than top";
                serializeJson(ackDoc, response);
                return response;
            }

            const bool saved = DB::saveProfile(bottom, top);
            Serial.printf("[INFO] Profile bottom=%.2f top=%.2f (%s)\n", bottom, top,
                          saved ? "saved" : "save FAILED");

            ackDoc["success"] = saved;
            ackDoc["message"] = saved ? "Profile saved" : "Failed to save profile";
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
                cb(CAL_TURBIDITY);

                return response;
            }
            else if (strcmp(sensorId, "tinggi_muka_air") == 0)
            {
                Serial.print("[INFO] MQTT CMD CALIBRATE TINGGI MUKA AIR");

                ackDoc["commandId"] = uuid;
                ackDoc["success"] = true;
                ackDoc["message"] = "START CALIBRATE TINGGI MUKA AIR";
                serializeJson(ackDoc, response);
                cb(CAL_WATER_LEVEL);

                return response;
            }
            else
            {
                Serial.printf("[ERROR] Unknown sensor ID: %s\n", sensorId);
                return "";
            }
        }
        else if (strcmp(type, "done-calibrate") == 0)
        {
            Serial.println("[INFO] MQTT CMD DONE CALIBRATE");

            ackDoc["commandId"] = uuid;
            ackDoc["success"] = true;
            ackDoc["message"] = "Done Calibrating";
            serializeJson(ackDoc, response);
            cb(CAL_UNKNOWN);

            return response;
        }
        else
        {
            Serial.printf("[ERROR] Unknown command type: %s\n", type);
            return "";
        }
    }
};

#endif // MQTT_CMD_PARSER_H
