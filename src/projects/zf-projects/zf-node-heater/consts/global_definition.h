#if !defined(GLOBAL_DEFINITION_H)
#define GLOBAL_DEFINITION_H

#include <Arduino.h>

namespace GlobalConfig
{
    constexpr uint8_t FLOOR_ID = 3;

    constexpr const char *ssid = "NodeSensorWiFi";
    constexpr const char *password = "muhammadnabiyullah";

    constexpr const char *MQTT_CREDENTIALS[] = {
        "zf-node-heater-1",
        "zf-node-heater-2",
        "zf-node-heater-3",
    };
    static_assert((FLOOR_ID - 1) < sizeof(MQTT_CREDENTIALS) / sizeof(MQTT_CREDENTIALS[0]),
                  "FLOOR_ID has no MQTT credentials entry");
    constexpr const char *usernameMQTT = MQTT_CREDENTIALS[FLOOR_ID - 1];
    constexpr const char *passwordMQTT = MQTT_CREDENTIALS[FLOOR_ID - 1];
    constexpr const char *brokerMQTT = "n12f87fb.ala.eu-central-1.emqxsl.com";

    constexpr uint8_t PIN_BUTTON = 4;
    constexpr uint8_t PIN_GREEN_LED = 12;
    constexpr uint8_t PIN_HEATER_RELAY = 5;
};

#endif // GLOBAL_DEFINITION_H
