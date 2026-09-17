#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <Stream.h>
#include <Wire.h>
#include "../utils/utils.h"
#include <projects/brin-bandung/slave-ws/utils/enum.h>

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
    const uint8_t pinRX = 20; // 20
    const uint8_t pinTX = 21; // 21

    contextMB(HardwareSerial &s) : serial(s) {}

    ModbusMessage FC03(ModbusMessage request)
    {
        uint16_t address;
        uint16_t words;
        ModbusMessage response;

        Serial.printf("[INFO] FC03 Req -> Server ID: %d, FC: %02X, Total Byte: %d\n",
                      request.getServerID(),
                      request.getFunctionCode(),
                      request.size());

        request.get(2, address); // Since slave id starts at bytes 2
        request.get(4, words);   // Since length address starts at bytes 4

        if (words > 0 && address + words <= 8)
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

        if (addr >= 8)
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

struct contextDaemon
{
    WiFiModule &wifi;
    FeatureStatus feature[4];
    Feature enabledFeature[4];

    contextDaemon(WiFiModule &w) : wifi(w) {}

    void setNTPStatus(FeatureStatus v) { feature[0] = v; }
    void setInternetStatus(FeatureStatus v) { feature[1] = v; }
    void setMQTTStatus(FeatureStatus v) { feature[2] = v; }
    void setWireGuardStatus(FeatureStatus v) { feature[3] = v; }

    FeatureStatus getNTPStatus() { return feature[0]; }
    FeatureStatus getInternetStatus() { return feature[1]; }
    FeatureStatus getMQTTStatus() { return feature[2]; }
    FeatureStatus getWireGuardStatus() { return feature[3]; }
};

#endif // TASK_CONTEXT_H
