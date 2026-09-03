#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>
#include <DFRobot_RainfallSensor.h>

#include "config.h" // .env
#include "transmitter/configs/wifi_module.h"
#include "../utils/utils.h"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "slave-arr-bandung-persemaian-1"; //! RECHECK THIS EVERYTIME COMPILE
WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);

AsyncWebServer server(80);
WireGuard wg;

/**
 * @brief Modbus address register mapping
 *  array[0] = (float) rainfall
 *  array[1-2] = (uint32_t) free heap
 *  array[3-4] = (uint32_t) largest block
 *  array[5-6] = (uint32_t) min heap
 *  array[7] = (uint8_t) reset reason
 */
ModbusServerTCPasync modbusServer;
const uint8_t MAX_REGISTER = 9;
uint16_t modbusData[MAX_REGISTER];
ModbusMessage FC03(ModbusMessage request);
ModbusMessage FC06(ModbusMessage request);

DFRobot_RainfallSensor_I2C rainSensor(&Wire);

/**
 * @brief Pinout note
 * - Slave ARR-1 (SDA = 5, SCL = 6)
 * - Slave ARR-2 (SDA = 7, SCL = 6)
 */
const uint8_t pinSDA = 5; // TODO: CHANGE TO APPROPRIATE PIN
const uint8_t pinSCL = 6; // TODO: CHANGE TO APPROPRIATE PIN

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

uint32_t prevTimeReading = 0;
uint32_t delayReading = 10000;
bool shouldRestartNow = false;
bool shouldResetRainfall = false;
bool hasResetToday = false;

void scanI2C();

void setup()
{
    Serial.begin(115200);

    wifi.begin(
        []() -> void
        { Serial.print("."); },
        []() -> void
        { esp_restart(); });

    esp_task_wdt_init(60, true);
    esp_task_wdt_add(NULL);

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onEnd([](bool success)
                     {if (success) esp_restart(); });
    ElegantOTA.begin(&server);
    WebSerial.begin(&server);
    server.begin();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Test Successful!"); });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
              { esp_restart(); });

    Wire.begin(pinSDA, pinSCL);
    if (rainSensor.begin())
    {
        Serial.println("Init sensor rainfall fail!");
        WebSerial.println("Init sensor rainfall fail!");
    }

    uint8_t retryCounter = 0;
    struct tm timeinfo;
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.google.com");
    while (!getLocalTime(&timeinfo) && retryCounter < 20)
    {
        Serial.print(".");
        delay(500);
        if (retryCounter >= 20)
        {
            Serial.println("\nNTP Sync Failed! Restarting...");
            WebSerial.println("\nNTP Sync Failed! Restarting...");
            esp_restart(); // Critical: WireGuard handshake will fail without correct time
        }
        retryCounter++;
    }

    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_SLAVE_ARR_LOCAL_IP_1);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_SLAVE_ARR_PRIVATE_KEY_1,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
    if (wgOk)
    {
        Serial.println("WireGuard successfully initialized on ESP32!");
        WebSerial.println("WireGuard successfully initialized on ESP32!");
    }
    else
    {
        Serial.println("WireGuard initialization failed!");
        WebSerial.println("WireGuard initialization failed!");
    }

    modbusServer.registerWorker(1, READ_HOLD_REGISTER, &FC03); // FC=03 for serverID=1
    modbusServer.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);
    modbusServer.start(5000, 2, 20000);
}

void loop()
{
    esp_task_wdt_reset();

    if (millis() - prevTimeReading > delayReading)
    {
        Serial.println("HEARTBEAT");
        WebSerial.println("HEARTBEAT");
        prevTimeReading = millis();
        scanI2C();

        // === SENSOR LOG DATA ===
        modbusData[0] = rainSensor.getRainfall(24); // return in mm/day

        // === SYSTEM LOG DATA ===
        uint32_t freeHeap = ESP.getFreeHeap();
        modbusData[1] = (uint16_t)(freeHeap >> 16);    // High word
        modbusData[2] = (uint16_t)(freeHeap & 0xFFFF); // Low word
        uint32_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        modbusData[3] = (uint16_t)(largestBlock >> 16);
        modbusData[4] = (uint16_t)(largestBlock & 0xFFFF);
        uint32_t minHeap = ESP.getMinFreeHeap();
        modbusData[5] = (uint16_t)(minHeap >> 16);
        modbusData[6] = (uint16_t)(minHeap & 0xFFFF);
        modbusData[7] = (uint16_t)esp_reset_reason();
        modbusData[8] = (fabs)(wifi.getRssi());

        struct tm timeinfo;
        getLocalTime(&timeinfo);
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
    }
}

ModbusMessage FC03(ModbusMessage request)
{
    /**
     * @brief Info about modbus TCP frame
     * | Slave id | Function code | Start Add | Length Add |
     * | 1 bytes  | 1 bytes       | 2 bytes   | 2 bytes    |
     */
    ModbusMessage response; // in
    uint16_t addr = 0;      // start address
    uint16_t words = 0;     // # of words requested
    request.get(2, addr);   // since start address is on bytes 3 of modbus frame
    request.get(4, words);  // since length address is on bytes 5 of modbus frame

    if ((addr + words) > MAX_REGISTER)
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);

    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    if (request.getFunctionCode() == READ_HOLD_REGISTER)
    {
        for (uint8_t i = 0; i < words; i++)
            response.add((uint16_t)modbusData[addr + i]);

        Serial.printf("Req Slave Id: %d, FC: %d, Data: [%d]\n",
                      request.getServerID(), request.getFunctionCode(),
                      modbusData[addr + 0]);

        WebSerial.printf("Req Slave Id: %d, FC: %d, Data: [%d]\n",
                         request.getServerID(), request.getFunctionCode(),
                         modbusData[addr + 0]);
    }
    return response;
}

ModbusMessage FC06(ModbusMessage request)
{
    ModbusMessage response;
    uint16_t addr = 0;  // Register address
    uint16_t value = 0; // Value to write

    request.get(2, addr);  // read address from request
    request.get(4, value); // read value from request

    Serial.printf("FC06: Write register %d = %d\n", addr, value);
    WebSerial.printf("FC06: Write register %d = %d\n", addr, value);

    if (addr >= 16)
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

void scanI2C()
{
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            WebSerial.printf("I2C device found at address 0x%02X\n", address);
            Serial.printf("I2C device found at address 0x%02X\n", address);
            nDevices++;
        }
        else if (error == 4)
        {
            WebSerial.printf("Unknown error at address 0x%02X\n", address);
            Serial.printf("Unknown error at address 0x%02X\n", address);
        }
    }

    if (nDevices == 0)
    {
        WebSerial.println("No I2C devices found\n");
        Serial.println("No I2C devices found\n");
    }
    else
    {
        WebSerial.println("I2C scan complete\n");
        Serial.println("I2C scan complete\n");
    }
}