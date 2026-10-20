#if !defined(HTTP_OTA_H)
#define HTTP_OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <Preferences.h>

class OTARequest
{
private:
    const bool _autoReboot = true;
    const bool _debugMode = false;

public:
    OTARequest(bool autoReboot, bool debug)
        : _autoReboot(autoReboot), _debugMode(debug) {}

    bool beginInsecure(const char *firmwareUrl, const char *currentVersion, const char *targetVersion)
    {
        WiFiClientSecure client;
        client.setInsecure();

        httpUpdate.onEnd([targetVersion]()
                         {
                             Preferences prefs;
                             if (!prefs.begin("app_config", false))
                                 return;
                             prefs.putString("version", targetVersion);
                             prefs.end(); });
        httpUpdate.rebootOnUpdate(false);

        t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl, currentVersion);
        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            if (_debugMode)
                Serial.printf("[ERROR] OTA Failed (Error: %d): %s\n",
                              httpUpdate.getLastError(),
                              httpUpdate.getLastErrorString().c_str());
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            if (_debugMode)
                Serial.printf("[INFO] No updates available\n");
            return false;
        case HTTP_UPDATE_OK:
            if (_debugMode)
                Serial.printf("[INFO] OTA Successful!");
            return true;
        }
        return false;
    }
};

#endif // HTTP_OTA_H
