#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <HardwareSerial.h>
#include <Wire.h>
#include <ModbusMessage.h>
#include "../../include/transmitter/configs/wifi_module.h"
#include "projects/brin-bandung/slave-arr/utils/enum.h"

#define MAX_REGISTER 16

volatile uint16_t sharedModbusData[MAX_REGISTER];

struct contextDaemon
{
    WiFiModule &wifi;
    volatile uint16_t *modbusData = sharedModbusData;
    FeatureStatus feature[4];
    Feature enabledFeature[4] = {FEATURE_DISABLED, FEATURE_DISABLED, FEATURE_DISABLED, FEATURE_DISABLED};

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

struct contextRainfall
{
    TwoWire &wire;
    const uint8_t pinSDA = 1;
    const uint8_t pinSCL = 0;

    contextRainfall(TwoWire &w) : wire(w) {}
};

struct contextMB
{
    HardwareSerial &serial;
    volatile uint16_t *modbusData = sharedModbusData;
    const uint8_t pinRX = 6;
    const uint8_t pinTX = 5;

    contextMB(HardwareSerial &s) : serial(s) {}

    ModbusMessage FC03(ModbusMessage request)
    {
        uint16_t address;
        uint16_t words;
        ModbusMessage response;

        request.get(2, address); // Since slave id starts at bytes 2
        request.get(4, words);   // Since length address starts at bytes 4

        Serial.printf("[INFO] FC03 Req -> Server ID: %d, FC: %02X, Total Byte: %d\n",
                      request.getServerID(),
                      request.getFunctionCode(),
                      request.size());

        if (words > 0 && (address + words) <= MAX_REGISTER)
        {
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
            for (uint16_t i = address; i < address + words; ++i)
                response.add(modbusData[i]);
            Serial.printf("[INFO] FC03 response prepared: %d bytes\n", response.size());
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

        Serial.printf("[INFO] FC06 Req -> Server ID: %d, FC: %02X, Reg: %d, Value: %d\n",
                      request.getServerID(),
                      request.getFunctionCode(),
                      addr,
                      value);

        if (addr >= MAX_REGISTER)
        {
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            return response;
        }
        modbusData[addr] = value;

        response.add(request.getServerID(), request.getFunctionCode());
        response.add(addr);
        response.add(value);

        return response;
    }

    ModbusMessage FC16(ModbusMessage request)
    {
        ModbusMessage response;
        uint16_t address = 0;
        uint16_t words = 0;
        uint8_t byteCount = 0;

        request.get(2, address);
        request.get(4, words);
        request.get(6, byteCount);

        if (words == 0 || byteCount != words * 2 || address + words > MAX_REGISTER)
        {
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            return response;
        }

        uint16_t offset = 7;
        for (uint16_t index = 0; index < words; ++index)
            offset = request.get(offset, modbusData[address + index]);

        response.add(request.getServerID(), request.getFunctionCode());
        response.add(address);
        response.add(words);
        return response;
    }
};

#endif // TASK_CONTEXT_H
