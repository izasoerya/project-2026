#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <HardwareSerial.h>
#include <ModbusMessage.h>
#include <Wire.h>
#include "../utils/utils.h"
#include "../utils/enum.h"
#include "wifi_bundle.h"

struct contextDaemon
{
    WiFiModule &wifi;
    FeatureStatus feature[4];

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

struct contextMBWS
{
    HardwareSerial &serial;
    static const size_t MAX_REGISTER = 8;
    volatile uint16_t data[MAX_REGISTER];
    const uint8_t pinRX = 3;
    const uint8_t pinTX = 4;
    uint8_t errorTransactionModbusCounter = 0;

    contextMBWS(HardwareSerial &s) : serial(s) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte
        offset = response.get(offset, data[0]);
        offset = response.get(offset, data[1]);
        offset = response.get(offset, data[2]);
        offset = response.get(offset, data[3]);
        offset = response.get(offset, data[4]);
        offset = response.get(offset, data[5]);
        offset = response.get(offset, data[6]);
        offset = response.get(offset, data[7]);
        errorTransactionModbusCounter = 0;
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        if (errorTransactionModbusCounter > 10)
            esp_restart();
        errorTransactionModbusCounter++;
    }
};

struct contextMBRainfall
{
    HardwareSerial &serial;
    static const size_t MAX_REGISTER = 8;
    volatile uint16_t data[MAX_REGISTER];
    const uint8_t pinRX = 5;
    const uint8_t pinTX = 6;
    uint8_t errorTransactionModbusCounter = 0;

    contextMBRainfall(HardwareSerial &s) : serial(s) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte
        offset = response.get(offset, data[0]);
        offset = response.get(offset, data[1]);
        offset = response.get(offset, data[2]);
        offset = response.get(offset, data[3]);
        offset = response.get(offset, data[4]);
        offset = response.get(offset, data[5]);
        offset = response.get(offset, data[6]);
        offset = response.get(offset, data[7]);
        errorTransactionModbusCounter = 0;
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        if (errorTransactionModbusCounter > 10)
            esp_restart();
        errorTransactionModbusCounter++;
    }
};

struct contextPublisher
{
    WiFiModule &wifi;
    const char *supabaseUrl = "https://pykernnkhvnssplhzcvn.supabase.co";
    const char *supabasePublicKey = "sb_publishable_coDPUa845ZtfYmoBWlZlgw_eH5vsCY7";
    SupabaseTransport transport = SupabaseTransport(supabaseUrl, supabasePublicKey);

    contextPublisher(WiFiModule &w) : wifi(w) {}
};

struct contextDisplay
{
    SPIClass &spi;
    const uint8_t pinSCK = 8;
    const uint8_t pinMISO = 20;
    const uint8_t pinMOSI = 9;
    const uint8_t pinCS = 5;

    contextDisplay(SPIClass &s) : spi(s) {}
};

#endif // TASK_CONTEXT_H
