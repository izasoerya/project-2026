#if !defined(SHARED_MODBUS_OBJECT_H)
#define SHARED_MODBUS_OBJECT_H

#include "transmitter/configs/mqtt_module.h"
#include "transmitter/configs/wifi_module.h"
#include "../consts/global_config.h"

namespace ModbusRegister
{
    constexpr uint16_t TIP_DAY = 0;        // (r)  tips since 00:00 (or current 24h window)
    constexpr uint16_t TIP_HALF_DAY = 1;   // (r)  tips since 00:00/12:00 (or current 12h window)
    constexpr uint16_t TIP_HOUR = 2;       // (r)  tips since top of hour (or current 1h window)
    constexpr uint16_t UNIX_TIME = 3;      // (rw) 3-4, uint32 UTC seconds, 0 = clock not set
    constexpr uint16_t RSSI = 5;           // (r)  int16 two's complement, dBm
    constexpr uint16_t FREE_HEAP = 6;      // (r)  6-7, uint32
    constexpr uint16_t MIN_FREE_HEAP = 8;  // (r)  8-9, uint32
    constexpr uint16_t LARGEST_BLOCK = 10; // (r)  10-11, uint32
    constexpr uint16_t RESET_REASON = 12;  // (r)  esp_reset_reason_t
    constexpr uint16_t SLAVE_ID = 13;      // (rw) modbus slave id
    constexpr uint16_t BAUD_RATE = 14;     // (rw) baud code, see BAUD_TABLE
    constexpr uint16_t RESTART = 15;       // (rw) write 1 to restart, reads 0

    constexpr size_t COUNT = GlobalConfig::MAX_REGISTER_MODBUS;
    static_assert(RESTART < COUNT, "MAX_REGISTER_MODBUS too small for register map");

    constexpr uint32_t BAUD_TABLE[] = {4800, 9600, 19200};
    constexpr uint16_t BAUD_COUNT = sizeof(BAUD_TABLE) / sizeof(BAUD_TABLE[0]);
    constexpr uint16_t BAUD_INVALID = 0xFFFF;

    inline uint16_t baudToCode(uint32_t baudRate)
    {
        for (uint16_t i = 0; i < BAUD_COUNT; i++)
            if (BAUD_TABLE[i] == baudRate)
                return i;
        return BAUD_INVALID;
    }

    inline uint32_t codeToBaud(uint16_t code)
    {
        return code < BAUD_COUNT ? BAUD_TABLE[code] : 0;
    }
}

struct ModbusContext
{
    static const uint8_t PIN_RX = 20;
    static const uint8_t PIN_TX = 21;

    uint8_t slaveId = GlobalConfig::SLAVE_ID;
    uint32_t baudRate = GlobalConfig::MODBUS_BAUD_RATE;
    uint32_t unixTime = 0; // last value published to UNIX_TIME, used to detect master writes

    ModbusContext() {}
};

struct SensorContext
{
    static constexpr uint8_t PIN_INT = 10;
    static constexpr uint32_t DEBOUNCE_MS = 0; // reed switch bounce filter

    // Fallback counting windows from startupMs, used while the clock is not set
    static constexpr uint32_t HOUR_MS = 60UL * 60UL * 1000UL;
    static constexpr uint32_t HALF_DAY_MS = 12UL * HOUR_MS;
    static constexpr uint32_t DAY_MS = 24UL * HOUR_MS;

    // Written by the tip ISR, guard read-modify-write outside it with `mux`
    volatile uint16_t tipHour = 0;
    volatile uint16_t tipHalfDay = 0;
    volatile uint16_t tipDay = 0;
    volatile uint32_t lastTipMs = 0;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Window tracking, only touched from the main loop
    uint64_t startupMs = 0;     // boot timestamp (64-bit, no 49-day wrap) for the fallback windows
    bool keysFromClock = false; // whether the keys below came from the clock or from millis
    int32_t dayKey = -1;        // current window ids, -1 = not initialised yet
    int32_t halfDayKey = -1;
    int32_t hourKey = -1;

    SensorContext() {}
};

struct NetworkingContext
{
    const char *ssid = GlobalConfig::ssid;
    const char *password = GlobalConfig::password;
    char hostname[32];

    WiFiModule &wifi;
    BaseMQTT &mqtt;

    NetworkingContext(WiFiModule &w, BaseMQTT &m) : wifi(w), mqtt(m)
    {
        snprintf(hostname, sizeof(hostname), "%s-%u", GlobalConfig::hostname, GlobalConfig::SLAVE_ID);
    }
};

#endif // SHARED_MODBUS_OBJECT_H