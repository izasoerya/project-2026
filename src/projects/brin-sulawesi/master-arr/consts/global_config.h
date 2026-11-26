#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

#ifndef DEVICE_ID
#define DEVICE_ID 1
#endif

#if DEVICE_ID < 1 || DEVICE_ID > 4
#error "DEVICE_ID must be between 1 and 4 for ARR"
#endif

#if DEVICE_ID == 1
#define ARR_DELAY_SAMPLING 10000
#define ARR_DELAY_TX 60000
#define ARR_LATITUDE -7.8367746
#define ARR_LONGITUDE 110.3421102
#define ARR_SSID "Subhanallah"
#define ARR_WIFI_PASSWORD "muhammadnabiyullah"
#define ARR_HOSTNAME "BRIN-ARR-DL-1"
#define ARR_USERNAME_MQTT "device_brin_sulawesi_arr"
#define ARR_PASSWORD_MQTT "4fc2ec90f50e065c0761055c6a7e15caaf87616a896b4292"
#define ARR_SENSOR_TOPIC "devices/BRIN-Sulawesi-ARR/data"
#define ARR_SENSOR_STREAM_TOPIC "devices/BRIN-Sulawesi-ARR/stream"
#define ARR_CMD_TOPIC "devices/BRIN-Sulawesi-ARR/cmd"
#define ARR_CMD_RESPONSE_TOPIC "devices/BRIN-Sulawesi-ARR/cmd/ack"
#define ARR_OTA_TOPIC "devices/BRIN-Sulawesi-ARR/ota"
#define ARR_OTA_RESPONSE_TOPIC "devices/BRIN-Sulawesi-ARR/ota/status"
#define ARR_PING_TOPIC "devices/BRIN-Sulawesi-ARR/ping"
#define ARR_PING_RESPONSE_TOPIC "devices/BRIN-Sulawesi-ARR/pong"
#elif DEVICE_ID == 2
#define ARR_DELAY_SAMPLING 0
#define ARR_DELAY_TX 0
#define ARR_LATITUDE 0.0
#define ARR_LONGITUDE 0.0
#define ARR_SSID "TODO_DEVICE_2_SSID"
#define ARR_WIFI_PASSWORD "TODO_DEVICE_2_WIFI_PASSWORD"
#define ARR_HOSTNAME "TODO_DEVICE_2_HOSTNAME"
#define ARR_USERNAME_MQTT "TODO_DEVICE_2_USERNAME"
#define ARR_PASSWORD_MQTT "TODO_DEVICE_2_PASSWORD"
#define ARR_SENSOR_TOPIC "TODO_DEVICE_2_SENSOR_TOPIC"
#define ARR_SENSOR_STREAM_TOPIC "TODO_DEVICE_2_SENSOR_STREAM_TOPIC"
#define ARR_CMD_TOPIC "TODO_DEVICE_2_CMD_TOPIC"
#define ARR_CMD_RESPONSE_TOPIC "TODO_DEVICE_2_CMD_RESPONSE_TOPIC"
#define ARR_OTA_TOPIC "TODO_DEVICE_2_OTA_TOPIC"
#define ARR_OTA_RESPONSE_TOPIC "TODO_DEVICE_2_OTA_RESPONSE_TOPIC"
#define ARR_PING_TOPIC "TODO_DEVICE_2_PING_TOPIC"
#define ARR_PING_RESPONSE_TOPIC "TODO_DEVICE_2_PING_RESPONSE_TOPIC"
#error "Fill in the ARR DEVICE_ID 2 configuration placeholders before building"
#elif DEVICE_ID == 3
#define ARR_DELAY_SAMPLING 0
#define ARR_DELAY_TX 0
#define ARR_LATITUDE 0.0
#define ARR_LONGITUDE 0.0
#define ARR_SSID "TODO_DEVICE_3_SSID"
#define ARR_WIFI_PASSWORD "TODO_DEVICE_3_WIFI_PASSWORD"
#define ARR_HOSTNAME "TODO_DEVICE_3_HOSTNAME"
#define ARR_USERNAME_MQTT "TODO_DEVICE_3_USERNAME"
#define ARR_PASSWORD_MQTT "TODO_DEVICE_3_PASSWORD"
#define ARR_SENSOR_TOPIC "TODO_DEVICE_3_SENSOR_TOPIC"
#define ARR_SENSOR_STREAM_TOPIC "TODO_DEVICE_3_SENSOR_STREAM_TOPIC"
#define ARR_CMD_TOPIC "TODO_DEVICE_3_CMD_TOPIC"
#define ARR_CMD_RESPONSE_TOPIC "TODO_DEVICE_3_CMD_RESPONSE_TOPIC"
#define ARR_OTA_TOPIC "TODO_DEVICE_3_OTA_TOPIC"
#define ARR_OTA_RESPONSE_TOPIC "TODO_DEVICE_3_OTA_RESPONSE_TOPIC"
#define ARR_PING_TOPIC "TODO_DEVICE_3_PING_TOPIC"
#define ARR_PING_RESPONSE_TOPIC "TODO_DEVICE_3_PING_RESPONSE_TOPIC"
#error "Fill in the ARR DEVICE_ID 3 configuration placeholders before building"
#elif DEVICE_ID == 4
#define ARR_DELAY_SAMPLING 0
#define ARR_DELAY_TX 0
#define ARR_LATITUDE 0.0
#define ARR_LONGITUDE 0.0
#define ARR_SSID "TODO_DEVICE_4_SSID"
#define ARR_WIFI_PASSWORD "TODO_DEVICE_4_WIFI_PASSWORD"
#define ARR_HOSTNAME "TODO_DEVICE_4_HOSTNAME"
#define ARR_USERNAME_MQTT "TODO_DEVICE_4_USERNAME"
#define ARR_PASSWORD_MQTT "TODO_DEVICE_4_PASSWORD"
#define ARR_SENSOR_TOPIC "TODO_DEVICE_4_SENSOR_TOPIC"
#define ARR_SENSOR_STREAM_TOPIC "TODO_DEVICE_4_SENSOR_STREAM_TOPIC"
#define ARR_CMD_TOPIC "TODO_DEVICE_4_CMD_TOPIC"
#define ARR_CMD_RESPONSE_TOPIC "TODO_DEVICE_4_CMD_RESPONSE_TOPIC"
#define ARR_OTA_TOPIC "TODO_DEVICE_4_OTA_TOPIC"
#define ARR_OTA_RESPONSE_TOPIC "TODO_DEVICE_4_OTA_RESPONSE_TOPIC"
#define ARR_PING_TOPIC "TODO_DEVICE_4_PING_TOPIC"
#define ARR_PING_RESPONSE_TOPIC "TODO_DEVICE_4_PING_RESPONSE_TOPIC"
#error "Fill in the ARR DEVICE_ID 4 configuration placeholders before building"
#endif

namespace GlobalConfig
{
    constexpr float BATTERY_DANGER = 11.5f;
    constexpr float BATTERY_WARNING = 11.8f;
    constexpr const char *BROKER_MQTT = "monitoring.pantaudas.cloud";
    constexpr uint16_t PORT_MQTT = 443;
    constexpr HardwareSerial &mbSerial = Serial1;
    constexpr uint8_t DEVICE_COUNT = 4;

    constexpr uint32_t DELAY_SAMPLING = ARR_DELAY_SAMPLING;
    constexpr uint32_t DELAY_TX = ARR_DELAY_TX;
    constexpr float latitude = ARR_LATITUDE;
    constexpr float longitude = ARR_LONGITUDE;
    constexpr const char *ssid = ARR_SSID;
    constexpr const char *password = ARR_WIFI_PASSWORD;
    constexpr const char *hostname = ARR_HOSTNAME;

    constexpr const char *USERNAME_MQTT = ARR_USERNAME_MQTT;
    constexpr const char *PASSWORD_MQTT = ARR_PASSWORD_MQTT;
    constexpr const char *SENSOR_TOPIC = ARR_SENSOR_TOPIC;
    constexpr const char *SENSOR_STREAM_TOPIC = ARR_SENSOR_STREAM_TOPIC;
    constexpr const char *CMD_TOPIC = ARR_CMD_TOPIC;
    constexpr const char *CMD_RESPONSE_TOPIC = ARR_CMD_RESPONSE_TOPIC;
    constexpr const char *OTA_TOPIC = ARR_OTA_TOPIC;
    constexpr const char *OTA_RESPONSE_TOPIC = ARR_OTA_RESPONSE_TOPIC;
    constexpr const char *PING_TOPIC = ARR_PING_TOPIC;
    constexpr const char *PING_RESPONSE_TOPIC = ARR_PING_RESPONSE_TOPIC;
}

#endif // CONFIG_H
