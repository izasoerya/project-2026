#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <HardwareSerial.h>
#include <ModbusMessage.h>
#include <Wire.h>
#include "../utils/utils.h"
#include "../utils/enum.h"
#include "wifi_bundle.h"

static volatile uint16_t sharedModbusData[8];

struct contextDaemon
{
    WiFiModule &wifi;
    FeatureStatus feature[4];
    volatile uint16_t *modbusData = sharedModbusData;

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
    volatile uint16_t *modbusData = sharedModbusData;
    const uint8_t pinRX = 21; // 21
    const uint8_t pinTX = 20; // 20
    uint8_t errorTransactionModbusCounter = 0;

    contextMBWS(HardwareSerial &s) : serial(s) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte
        offset = response.get(offset, modbusData[0]);
        offset = response.get(offset, modbusData[1]);
        offset = response.get(offset, modbusData[2]);
        offset = response.get(offset, modbusData[3]);
        offset = response.get(offset, modbusData[4]);
        offset = response.get(offset, modbusData[5]);
        offset = response.get(offset, modbusData[6]);
        offset = response.get(offset, modbusData[7]);
        errorTransactionModbusCounter = 0;

        Serial.print("[INFO] MBWS Incoming FC03: ");
        for (size_t i = 0; i < response.size(); i++)
            Serial.printf("%02X ", response[i]);
        Serial.println();
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        Serial.printf("[ERROR] MBWS modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        errorTransactionModbusCounter++;
    }
};

struct contextMBRainfall
{
    HardwareSerial &serial;
    volatile uint16_t *modbusData = sharedModbusData;
    const uint8_t pinRX = 8;
    const uint8_t pinTX = 9;
    uint8_t errorTransactionModbusCounter = 0;

    contextMBRainfall(HardwareSerial &s) : serial(s) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte
        offset = response.get(offset, modbusData[0]);
        offset = response.get(offset, modbusData[1]);
        offset = response.get(offset, modbusData[2]);
        offset = response.get(offset, modbusData[3]);
        offset = response.get(offset, modbusData[4]);
        offset = response.get(offset, modbusData[5]);
        offset = response.get(offset, modbusData[6]);
        offset = response.get(offset, modbusData[7]);
        errorTransactionModbusCounter = 0;

        Serial.print("[INFO] MBRain Incoming FC03: ");
        for (size_t i = 0; i < response.size(); i++)
            Serial.printf("%02X ", response[i]);
        Serial.println();
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        Serial.printf("[ERROR] MBRain modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        errorTransactionModbusCounter++;
    }
};

struct contextPublisher
{
    WiFiModule &wifi;
    const char *supabaseUrl = "https://gothabjdasaphwzrjnto.supabase.co";
    const char *supabasePublicKey = "sb_publishable_Dx3vXSh8qdQhM1Zi_V1MTQ_ifqdbX1o";
    SupabaseTransport transport = SupabaseTransport(supabaseUrl, supabasePublicKey);

    contextPublisher(WiFiModule &w) : wifi(w) {}
};

struct contextDisplay
{
    SPIClass &spi;
    const uint8_t pinSCK = 1;
    const uint8_t pinMISO = 10;
    const uint8_t pinMOSI = 0;
    const uint8_t pinCS = 5;

    contextDisplay(SPIClass &s) : spi(s) {}
};

#endif // TASK_CONTEXT_H
