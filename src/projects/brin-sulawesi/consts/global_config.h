#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

namespace GlobalConfig
{
    constexpr uint8_t deviceID = 1;

    constexpr const char *ssid = "NodeSensorWiFi1";
    constexpr const char *password = "muhammadnabiyullah";
    constexpr const char *hostname = "BRIN-WTQ-DL";

    constexpr const char *usernameMqtt = "";
    constexpr const char *passwordMqtt = "";
    constexpr const char *brokerMqtt = "";
    constexpr const uint16_t portMqtt = 1883;
    constexpr const char *SENSOR_TOPIC = "";

};

#endif // CONFIG_H
