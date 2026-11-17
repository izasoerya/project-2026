#if !defined(MODBUS_UTILITY_H)
#define MODBUS_UTILITY_H

#include <Arduino.h>

class ModbusUtility
{
public:
    static uint32_t decodeUint32(uint16_t highWord, uint16_t lowWord)
    {
        return (static_cast<uint32_t>(highWord) << 16) | lowWord;
    }
};

#endif // MODBUS_UTILITY_H