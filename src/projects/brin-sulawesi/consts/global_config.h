#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

namespace GlobalConfig
{
    float BATTERY_DANGER = 11.5;
    float BATTERY_WARNING = 11.8;

    constexpr uint8_t deviceID = 1;
    constexpr uint32_t DELAY_SAMPLING = 10000;
    constexpr uint32_t DELAY_TX = 60000;

    constexpr const char *ssid = "Subhanallah";
    constexpr const char *password = "muhammadnabiyullah";
    constexpr const char *hostname = "BRIN-WTQ-DL";

    constexpr const char *USERNAME_MQTT = "device_tester_water_sukawesi";
    constexpr const char *PASSWORD_MQTT = "7d4a91651108b30afb438f24f3b8b3b15abfe20d6a35c0c3";
    constexpr const char *BROKER_MQTT = "monitoring.pantaudas.cloud";
    constexpr const uint16_t PORT_MQTT = 443;

    constexpr const char *SENSOR_TOPIC = "devices/tester-water-sukawesi/data";
    constexpr const char *SENSOR_STREAM_TOPIC = "devices/tester-water-sukawesi/stream";
    constexpr const char *CMD_TOPIC = "devices/tester-water-sukawesi/cmd";
    constexpr const char *CMD_RESPONSE_TOPIC = "devices/tester-water-sukawesi/cmd/ack";
    constexpr const char *OTA_TOPIC = "devices/tester-water-sukawesi/ota";
    constexpr const char *OTA_RESPONSE_TOPIC = "devices/tester-water-sukawesi/ota/status";
    constexpr const char *PING_TOPIC = "devices/tester-water-sukawesi/ping";
    constexpr const char *PING_RESPONSE_TOPIC = "devices/tester-water-sukawesi/pong";

    constexpr HardwareSerial &mbSerial = Serial1;
};

#endif // CONFIG_H
