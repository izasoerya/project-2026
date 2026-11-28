#if !defined(SLAVE_REGISTERS_H)
#define SLAVE_REGISTERS_H

#include <Arduino.h>

/**
 * Holding register map of the ARR slave (slave-arr).
 * MUST match ModbusRegister in slave-arr/models/task_context.h.
 * 32-bit values span two registers, high word first (see BitMask).
 */
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

    constexpr uint16_t COUNT = 16;

    // Baud code <-> baud rate. Index is the value stored in BAUD_RATE register.
    constexpr uint32_t BAUD_TABLE[] = {4800, 9600, 19200};
}

#endif // SLAVE_REGISTERS_H
