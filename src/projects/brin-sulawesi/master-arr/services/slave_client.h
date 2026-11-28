#if !defined(SLAVE_CLIENT_H)
#define SLAVE_CLIENT_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include "../consts/global_config.h"
#include "../consts/slave_registers.h"
#include "../utils/bitmask.h"

/**
 * Writes to the ARR slave's (rw) registers.
 * Reads go through Modbustatics (SensorContext::rainFall), ModbusBuilder is read-only.
 */
class SlaveClient
{
private:
    static ModbusMaster &node(uint8_t slaveId)
    {
        static ModbusMaster instance;
        instance.begin(slaveId, GlobalConfig::mbSerial);
        return instance;
    }

    static bool check(uint8_t result, const char *what)
    {
        if (result == ModbusMaster::ku8MBSuccess)
            return true;
        Serial.printf("[ERROR] Slave %s failed, modbus error 0x%02X\n", what, result);
        return false;
    }

public:
    // Write UTC unix time to UNIX_TIME (3-4) in one FC16 request, as the slave requires
    static bool writeUnixTime(uint8_t slaveId, uint32_t unixTime)
    {
        uint16_t regs[2];
        BitMask::encodeU32(unixTime, regs);

        ModbusMaster &mb = node(slaveId);
        mb.clearTransmitBuffer();
        mb.setTransmitBuffer(0, regs[0]);
        mb.setTransmitBuffer(1, regs[1]);
        return check(mb.writeMultipleRegisters(ModbusRegister::UNIX_TIME, 2), "time sync");
    }

    // Ask the slave to restart (it acks first, then reboots)
    static bool restart(uint8_t slaveId)
    {
        return check(node(slaveId).writeSingleRegister(ModbusRegister::RESTART, 1), "restart");
    }
};

#endif // SLAVE_CLIENT_H
