#if !defined(CONFIG_H)
#define CONFIG_H

#include <Arduino.h>

#ifndef DEVICE_SLAVE_ID
#define DEVICE_SLAVE_ID 1
#endif

namespace GlobalConfig
{
    constexpr const char *ssid = "Subhanallah";
    constexpr const char *password = "muhammadnabiyullah";
    constexpr const char *hostname = "slave-arr";
    constexpr const char *BROKER_MQTT = "monitoring.pantaudas.cloud";
    constexpr uint16_t PORT_MQTT = 443;

    constexpr uint8_t SLAVE_ID = DEVICE_SLAVE_ID;
    constexpr uint32_t MODBUS_BAUD_RATE = 9600;
    constexpr const uint16_t MAX_REGISTER_MODBUS = 16; // register count (0..15), see ModbusRegister
    HardwareSerial &mbSerial = Serial1;

    // Clock (set by the master through the UNIX_TIME registers)
    constexpr const char *TIMEZONE = "WITA-8";        // POSIX TZ, UTC+8 (Sulawesi)
    constexpr uint32_t MIN_VALID_UNIX = 1704067200UL; // 2024-01-01 00:00 UTC, older = clock not set
}

#endif // CONFIG_H
