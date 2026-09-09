#if !defined(NTP_SERVICE)
#define NTP_SERVICE

#include <Arduino.h>
#include <WiFi.h>

struct TimeStruct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;

    bool isValid()
    {
        if (this->hour == 255)
            return false;
        return true;
    }
};

class NTPService
{
public:
    static bool init()
    {
        const char *ntpServer = "pool.ntp.org";
        const uint16_t gmtOffset_sec = 25200;
        const uint16_t daylightOffset_sec = 0;

        if (WiFi.isConnected())
            return false;
        uint8_t retryCounter = 0;
        struct tm timeinfo;
        configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.google.com");
        while (!getLocalTime(&timeinfo) && retryCounter < 20)
        {
            Serial.print(".");
            delay(500);
            if (retryCounter >= 20)
            {
                Serial.println("\nNTP Sync Failed! Restarting...");
                return false; // Critical: WireGuard handshake will fail without correct time
            }
            retryCounter++;
        }
        return true;
    }

    static TimeStruct getTime()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            return TimeStruct{.second = 0, .minute = 0, .hour = 255};
        }

        return TimeStruct{
            .second = static_cast<unsigned char>(timeinfo.tm_sec),
            .minute = static_cast<unsigned char>(timeinfo.tm_min),
            .hour = static_cast<unsigned char>(timeinfo.tm_hour)};
    }
};

#endif // NTP_SERVICE
