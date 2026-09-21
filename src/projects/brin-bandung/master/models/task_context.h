#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include <HardwareSerial.h>
#include <ModbusMessage.h>
#include <ModbusClientRTU.h>
#include <RTUutils.h>
#include <Wire.h>
#include <functional>
#include "../utils/utils.h"
#include "../utils/enum.h"
#include "wifi_bundle.h"

static volatile uint16_t sharedModbusDataWS[16];
static volatile uint16_t sharedModbusDataARR[16];

struct sharedDelayContext
{
    uint32_t delay = 60000;
};

struct sharedModbusClientContext
{
    const uint8_t pinRX = 20;
    const uint8_t pinTX = 21;
    HardwareSerial &serial;
    ModbusClientRTU mb;

    SemaphoreHandle_t _mutex;
    bool initialized = false;
    std::function<void(ModbusMessage, uint32_t)> wsDataHandler;
    std::function<void(ModbusMessage, uint32_t)> rainfallDataHandler;
    std::function<void(Error, uint32_t)> wsErrorHandler;
    std::function<void(Error, uint32_t)> rainfallErrorHandler;

    sharedModbusClientContext(HardwareSerial &s)
        : serial(s), _mutex(xSemaphoreCreateMutex())
    {
        mb.onDataHandler([this](ModbusMessage response, uint32_t token)
                         {
                             if (response.getServerID() == 1 && wsDataHandler)
                                 wsDataHandler(response, token);
                             else if (response.getServerID() == 2 && rainfallDataHandler)
                                 rainfallDataHandler(response, token); });
        mb.onErrorHandler([this](Error error, uint32_t token)
                          {
                                  if ((token & 1) && wsErrorHandler)
                                      wsErrorHandler(error, token);
                                  else if (!(token & 1) && rainfallErrorHandler)
                                      rainfallErrorHandler(error, token); });
    }

    bool begin()
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        if (initialized)
        {
            xSemaphoreGive(_mutex);
            return true;
        }

        RTUutils::prepareHardwareSerial(serial);
        serial.begin(9600, SERIAL_8N1, pinRX, pinTX);
        mb.begin(serial);
        initialized = true;
        xSemaphoreGive(_mutex);
        return true;
    }

    Error addRequest(uint32_t token, uint8_t serverId,
                     uint8_t functionCode, uint16_t address, uint16_t value)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        Error error = mb.addRequest(token, serverId, functionCode, address, value);
        xSemaphoreGive(_mutex);
        return error;
    }

    Error addRequests(uint32_t token, uint8_t serverId,
                      uint8_t functionCode, uint16_t address,
                      uint16_t wordCount, uint8_t byteCount,
                      uint16_t *data)
    {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        Error error = mb.addRequest(token, serverId, functionCode,
                                    address, wordCount, byteCount, data);
        xSemaphoreGive(_mutex);
        return error;
    }
};

static sharedDelayContext contextSharedDelay;

struct contextMBWS
{
    sharedModbusClientContext &sharedClient;
    volatile uint16_t *modbusData = sharedModbusDataWS;
    uint8_t errorTransactionModbusCounter = 0;

    contextMBWS(HardwareSerial &s, sharedModbusClientContext &client)
        : sharedClient(client) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        if (response.getFunctionCode() != READ_HOLD_REGISTER)
        {
            Serial.printf("[INFO] MBWS response ignored, FC: %02X\n",
                          response.getFunctionCode());
            return;
        }

        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte

        // ADDRESS 0 - 4 ARE FOR SENSOR DATA
        offset = response.get(offset, modbusData[0]); // T
        offset = response.get(offset, modbusData[1]); // H
        offset = response.get(offset, modbusData[2]); // WS
        offset = response.get(offset, modbusData[3]); // WD
        offset = response.get(offset, modbusData[4]); // IC Temperature
        //  ADDRES 5 - 8 ARE FOR CONFIGURATION
        offset = response.get(offset, modbusData[5]); // DELAY REQ
        offset = response.get(offset, modbusData[6]); // DEBUG STATE (INET, OTA, WEBSER)
        offset = response.get(offset, modbusData[7]); // RESTART

        errorTransactionModbusCounter = 0;

        SensorWSObject sensorWS{
            .temperature = modbusData[0] / 10.0F,
            .humidity = modbusData[1] / 10.0F,
            .windSpeed = modbusData[2] / 10.0F,
            .windDirection = static_cast<WindDirectionEnum>(modbusData[3]),
        };
        Serial.printf("[INFO] FC03 MBWS Sensor: %s | IC Temp: %.1f\n", sensorWS.toString(), modbusData[4] / 10.0F);
        WebSerial.printf("[INFO] FC03 MBWS Sensor: %s | IC Temp: %.1f\n", sensorWS.toString(), modbusData[4] / 10.0F);
        xQueueSend(queueSensorWS, &sensorWS, pdMS_TO_TICKS(10));
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        Serial.printf("[ERROR] MBWS modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        WebSerial.printf("[ERROR] MBWS modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        errorTransactionModbusCounter++;
    }
};

struct contextMBRainfall
{
    sharedModbusClientContext &sharedClient;
    volatile uint16_t *modbusData = sharedModbusDataARR;
    uint8_t errorTransactionModbusCounter = 0;

    contextMBRainfall(HardwareSerial &s, sharedModbusClientContext &client)
        : sharedClient(client) {}

    void onDataIncoming(ModbusMessage response, uint32_t token)
    {
        if (response.getFunctionCode() != READ_HOLD_REGISTER)
        {
            Serial.printf("[INFO] MBRain response ignored, FC: %02X\n",
                          response.getFunctionCode());
            return;
        }

        uint16_t offset = 3; // First value is on pos 3, after server ID, function code and length byte

        // ADDRESS 0 - 4 ARE FOR SENSOR DATA
        offset = response.get(offset, modbusData[0]); // Rain in mm

        //  ADDRES 5 - 8 ARE FOR CONFIGURATION
        offset = response.get(offset, modbusData[5]); // DELAY REQ
        offset = response.get(offset, modbusData[6]); // DEBUG STATE (INET, OTA, WEBSER)
        offset = response.get(offset, modbusData[7]); // RESTARTs
        errorTransactionModbusCounter = 0;

        SensorRainfallObject sensorRainfall{
            .rainfall = modbusData[0] / 10.0F,
        };
        xQueueSend(queueSensorRainfall, &sensorRainfall, pdMS_TO_TICKS(10));
        Serial.printf("[INFO] FC03 ARR Sensor: %s\n", sensorRainfall.toString());
        WebSerial.printf("[INFO] FC03 ARR Sensor: %s\n", sensorRainfall.toString());
    }

    void onErrorHandler(Error error, uint32_t token)
    {
        Serial.printf("[ERROR] MBRain modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        WebSerial.printf("[ERROR] MBRain modbus receive: %s | %d\n", String(error), errorTransactionModbusCounter);
        errorTransactionModbusCounter++;
    }
};

struct contextPublisher
{
    WiFiModule &wifi;
    const char *supabaseUrl = "https://pykernnkhvnssplhzcvn.supabase.co";
    const char *supabasePublicKey = "sb_publishable_coDPUa845ZtfYmoBWlZlgw_eH5vsCY7";
    const char *tableSensor = "sensors";
    const char *tableSystemLlogs = "system_logs";
    SupabaseTransport transport = SupabaseTransport(supabaseUrl, supabasePublicKey);
    sharedDelayContext *delay = &contextSharedDelay;

    contextPublisher(WiFiModule &w) : wifi(w) {}
};

struct contextDisplay
{
    SPIClass &spi;
    const uint8_t pinSCK = 9;
    const uint8_t pinMISO = 10;
    const uint8_t pinMOSI = 8;
    const uint8_t pinCS = 5;

    contextDisplay(SPIClass &s) : spi(s) {}
};

struct contextDaemon
{
    WiFiModule &wifi;
    FeatureStatus feature[4];
    volatile uint16_t *modbusDataWS = sharedModbusDataWS;
    volatile uint16_t *modbusDataARR = sharedModbusDataARR;
    sharedDelayContext *delay = &contextSharedDelay;

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
