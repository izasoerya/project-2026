#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

namespace GlobalConfig
{
    constexpr uint8_t deviceID = 1;
    constexpr uint32_t DELAY_SAMPLING = 5000;
    constexpr uint32_t DELAY_TX = 60000;

    constexpr const char *ssid = "Subhanallah";
    constexpr const char *password = "muhammadnabiyullah";
    constexpr const char *hostname = "BRIN-WTQ-DL";

    constexpr const char *usernameMqtt = "test2";
    constexpr const char *passwordMqtt = "test";
    constexpr const char *brokerMqtt = "n12f87fb.ala.eu-central-1.emqxsl.com";
    constexpr const uint16_t portMqtt = 8883;
    constexpr const char *SENSOR_TOPIC = "sensor";

};

#endif // CONFIG_H
