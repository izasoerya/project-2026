#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <Stream.h>
#include "../utils/utils.h"
#include <Wire.h>

struct contextWD
{
    Stream &serial;
    const uint8_t pinRX = 4;
    const uint8_t pinTX = 3;

    contextWD(Stream &s) : serial(s) {}
};

struct contextMB
{
    HardwareSerial &serial;
    volatile uint16_t data[8];
    const uint8_t pinRX = 5;
    const uint8_t pinTX = 6;

    contextMB(HardwareSerial &s) : serial(s) {}

    ModbusMessage FC03(ModbusMessage request)
    {
        uint16_t address;
        uint16_t words;
        ModbusMessage response;

        request.get(2, address); // Since slave id starts at bytes 2
        request.get(4, words);   // Since length address starts at bytes 4

        if (address && words && (address + words) <= 10)
        {
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
            for (uint16_t i = address; i < address + words; ++i)
                response.add(data[i]);
        }
        else
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);

        return response;
    }

    ModbusMessage FC06(ModbusMessage request)
    {
        ModbusMessage response;
        uint16_t addr = 0;  // Register address
        uint16_t value = 0; // Value to write

        request.get(2, addr);  // read address from request
        request.get(4, value); // read value from request

        if (addr >= 16)
        {
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            return response;
        }
        data[addr] = value;

        response.add(request.getServerID(), request.getFunctionCode());
        response.add(addr);
        response.add(value);

        return response;
    }
};

struct contextTHWS
{
    TwoWire &wire;
    const uint8_t pinSDA = 7;
    const uint8_t pinSCL = 8;

    contextTHWS(TwoWire &w) : wire(w) {}
};

#endif // TASK_CONTEXT_H
