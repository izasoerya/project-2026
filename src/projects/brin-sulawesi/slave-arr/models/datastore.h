#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

#include <ArduinoJson.h>
#include <esp_system.h>
#include "../utils/bitmask.h"
#include "task_context.h" // ModbusRegister map

struct SensorObject
{
    uint16_t tippingDay;
    uint16_t tippingHalfDay;
    uint16_t tippingHour;

    const char *toString()
    {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer),
                 "Tip/d: %u | Tip/12h: %u | Tip/h: %u\n",
                 tippingDay, tippingHalfDay, tippingHour);
        return buffer;
    }

    size_t toArray(uint16_t *datas)
    {
        datas[0] = tippingDay;
        datas[1] = tippingHalfDay;
        datas[2] = tippingHour;
        return 3;
    }
};

struct ModbusSensorAgregate
{
    SensorObject _sensor;
    int8_t _rssi = 0;
    uint8_t _slaveId = 0;
    uint32_t _baudRate = 0;
    uint32_t _unixTime = 0;

    ModbusSensorAgregate(SensorObject s) : _sensor(s) {}

    void setUnixTime(uint32_t unixTime) { _unixTime = unixTime; }

    void setSlaveId(uint8_t slaveId) { _slaveId = slaveId; }

    void setBaud(uint32_t baud) { _baudRate = baud; }

    void setRSSI(int8_t rssi) { _rssi = rssi; }

    const char *toString()
    {
        static char buffer[192];
        snprintf(buffer, sizeof(buffer),
                 "Tip/d: %u | Tip/12h: %u | Tip/h: %u | UNIX: %lu | RSSI: %d | ID: %u | BAUD: %lu | "
                 "F_HEAP: %lu | MIN_F_HEAP: %lu | LARGEST_BLOCK: %lu\n",
                 _sensor.tippingDay, _sensor.tippingHalfDay, _sensor.tippingHour,
                 (unsigned long)_unixTime, _rssi, _slaveId, (unsigned long)_baudRate,
                 (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
                 (unsigned long)ESP.getMaxAllocHeap());
        return buffer;
    }

    /**
     * Fill the full register map (see ModbusRegister in task_context.h).
     * @param datas buffer of at least ModbusRegister::COUNT registers
     * @return number of registers written (ModbusRegister::COUNT)
     */
    size_t toArray(uint16_t *datas)
    {
        using namespace ModbusRegister;

        // Sensor
        datas[TIP_DAY] = _sensor.tippingDay;
        datas[TIP_HALF_DAY] = _sensor.tippingHalfDay;
        datas[TIP_HOUR] = _sensor.tippingHour;

        // Clock
        BitMask::encodeU32(_unixTime, &datas[UNIX_TIME]);

        // System log
        datas[RSSI] = static_cast<uint16_t>(static_cast<int16_t>(_rssi));
        BitMask::encodeU32(ESP.getFreeHeap(), &datas[FREE_HEAP]);
        BitMask::encodeU32(ESP.getMinFreeHeap(), &datas[MIN_FREE_HEAP]);
        BitMask::encodeU32(ESP.getMaxAllocHeap(), &datas[LARGEST_BLOCK]);
        datas[RESET_REASON] = static_cast<uint16_t>(esp_reset_reason());

        // Config
        datas[SLAVE_ID] = _slaveId;
        datas[BAUD_RATE] = baudToCode(_baudRate);
        datas[RESTART] = 0;

        return COUNT;
    }
};

#endif // SENSOR_DATASTORE_H
