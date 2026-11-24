#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

namespace GlobalConfig
{
    float BATTERY_DANGER = 11.5;
    float BATTERY_WARNING = 11.8;

    constexpr uint8_t DEVICE_ID = 1;
    constexpr uint8_t DEVICE_COUNT = 4;
    constexpr uint32_t DELAY_SAMPLING = 10000;
    constexpr uint32_t DELAY_TX = 60000;

    constexpr const float latitude = -7.8367746;
    constexpr const float longitude = 110.3421102;

    constexpr const char *ssid = "Subhanallah";
    constexpr const char *password = "muhammadnabiyullah";
    constexpr const char *hostname = "BRIN-ARR-DL";

    constexpr const char *USERNAME_MQTT = "device_brin_sulawesi_arr";
    constexpr const char *PASSWORD_MQTT = "4fc2ec90f50e065c0761055c6a7e15caaf87616a896b4292";
    constexpr const char *BROKER_MQTT = "monitoring.pantaudas.cloud";
    constexpr const uint16_t PORT_MQTT = 443;

    constexpr const char *SENSOR_TOPIC = "devices/BRIN-Sulawesi-ARR/data";
    constexpr const char *SENSOR_STREAM_TOPIC = "devices/BRIN-Sulawesi-ARR/stream";
    constexpr const char *CMD_TOPIC = "devices/BRIN-Sulawesi-ARR/cmd";
    constexpr const char *CMD_RESPONSE_TOPIC = "devices/BRIN-Sulawesi-ARR/cmd/ack";
    constexpr const char *OTA_TOPIC = "devices/BRIN-Sulawesi-ARR/ota";
    constexpr const char *OTA_RESPONSE_TOPIC = "devices/BRIN-Sulawesi-ARR/ota/status";
    constexpr const char *PING_TOPIC = "devices/BRIN-Sulawesi-ARR/ping";
    constexpr const char *PING_RESPONSE_TOPIC = "devices/BRIN-Sulawesi-ARR/pong";

    constexpr HardwareSerial &mbSerial = Serial1;
};

#endif // CONFIG_H
