#if !defined(GLOBAL_DEFINITION_H)
#define GLOBAL_DEFINITION_H

#include <Arduino.h>

namespace GlobalConfig
{
    constexpr uint8_t FLOOR_ID = 2;

    constexpr const char *ssid = "Subhanallah";
    constexpr const char *password = "muhammadnabiyullah";

    constexpr const char *usernameMQTT = "test2";
    constexpr const char *passwordMQTT = "test";
    constexpr const char *brokerMQTT = "n12f87fb.ala.eu-central-1.emqxsl.com";

    constexpr uint8_t PIN_BUTTON = 4;
    constexpr uint8_t PIN_GREEN_LED = 12;
    constexpr uint8_t PIN_HEATER_RELAY = 5;
};

#endif // GLOBAL_DEFINITION_H
