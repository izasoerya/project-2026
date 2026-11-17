#if !defined(MODBUS_UTILITY_H)
#define MODBUS_UTILITY_H

#include <Arduino.h>

class ModbusUtility
{
public:
    static void encodeUint32(uint32_t value, uint16_t &highWord, uint16_t &lowWord)
    {
        highWord = (uint16_t)(value >> 16); // Upper 16 bits
        lowWord = (uint16_t)value;          // Lower 16 bits
    }

    // DECODE - Combine two uint16 registers back to uint32
    static uint32_t decodeUint32(uint16_t highWord, uint16_t lowWord)
    {
        return ((uint32_t)highWord << 16) | lowWord;
    }
};

#endif // MODBUS_UTILITY_H
