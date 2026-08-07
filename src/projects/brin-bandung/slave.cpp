#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <SHT2x.h> //TODO: REFACTOR TO SHT30D
#include <WireGuard-ESP32.h>

#include "config.h" // .env
#include "models.h"
#include "transmitter/configs/wifi_module.h"
#include "../utils/utils.h"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "slave-bandung-persemaian-1";
WiFiModule wifi(ssid, password, hostname);

AsyncWebServer server(80);
WireGuard wg;

ModbusServerTCPasync modbusServer;
const uint8_t MAX_REGISTER = 16;
uint16_t modbusData[MAX_REGISTER];
ModbusMessage FC03(ModbusMessage request);
ModbusMessage FC06(ModbusMessage request);

const uint8_t pinAnemo = 9;
const uint8_t pinRainfall = 10;
const uint8_t pinWindDirectionRX = 5; // TODO: CHECK DOUBLE PLZ
const uint8_t pinWindDirectionTX = 6; // TODO: CHECK DOUBLE PLZ
const uint8_t pinSDA = 7;
const uint8_t pinSCL = 8;

SHT2x sht;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

volatile uint32_t counterAnemo = 0;
const uint16_t debounceAnemo = 5000; // milisecond
uint32_t prevDebounceAnemo = 0;

volatile uint32_t counterRainfall = 0;
const uint8_t debounceRainfall = 100; // milisecond
uint32_t prevDebounceRainfall = 0;

uint32_t prevTimeReading = 0;
uint32_t delayReading = 10000;
bool shouldRestartNow = false;
bool shouldResetRainfall = false;
bool hasResetToday = false;

void ARDUINO_ISR_ATTR rainfallInterruptHandler();
void ARDUINO_ISR_ATTR anemoInterruptHandler();

void setup()
{
    Serial.begin(115200);
    // Serial1.begin(9600, SERIAL_8N1, pinWindDirectionTX, pinWindDirectionRX);

    if (wifi.begin())
        Serial.println(wifi.localIP());

    esp_task_wdt_init(60, true);
    esp_task_wdt_add(NULL);

    pinMode(pinAnemo, INPUT_PULLUP);
    pinMode(pinRainfall, INPUT);

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server);
    WebSerial.begin(&server);
    server.begin();

    Wire.begin(pinSDA, pinSCL);
    if (!sht.begin())
    {
        Serial.println("SHTX is not working");
        WebSerial.println("SHTX is not working");
    }

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    detachInterrupt(pinAnemo);
    detachInterrupt(pinRainfall);
    attachInterrupt(pinAnemo, anemoInterruptHandler, RISING);
    attachInterrupt(pinRainfall, rainfallInterruptHandler, RISING);

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
            ESP.restart(); // Critical: WireGuard handshake will fail without correct time
        }
        retryCounter++;
    }

    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_SLAVE_LOCAL_IP_1);
    Serial.printf("wg ip: %s", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_SLAVE_PRIVATE_KEY_1,
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
    modbusData[12] = delayReading;
}

void loop()
{
    esp_task_wdt_reset();

    if (millis() - prevTimeReading > delayReading)
    {
        Serial.println("HEARTBEAT");
        WebSerial.println("HEARTBEAT");
        prevTimeReading = millis();

        // === SENSOR DATA ===
        // if (Serial1.available())
        // {
        //     String data = Serial1.readString(); // data yang diterima dari sensor berawalan tanda * dan diakhiri tanda #, contoh *1#
        //     int a = data.indexOf("*");          // a adalah index tanda *
        //     int b = data.indexOf("#");          // b adalah index tanda #
        //     String resultWind = data.substring(a + 1, b);
        //     modbusData[4] = Parser::parseStringWindDirection(resultWind);
        // }

        // modbusData[0] = sht.getTemperature();
        // modbusData[1] = sht.getHumidity();

        float rainFallResult = counterRainfall * 0.7; // Return in mm/<time>
        float anemoResult = (-0.0181 * float(counterAnemo / delayReading) * float(counterAnemo / delayReading)) +
                            (1.3859 * float(counterAnemo / delayReading)) + 1.4055; // Return in m/s
        modbusData[2] = uint16_t(rainFallResult * 10);                              // Store as uint and .1 precision
        modbusData[3] = uint16_t(anemoResult * 10);                                 // Store as uint and .1 precision

        modbusData[0] = random(1250);
        modbusData[1] = random(1000);
        // modbusData[2] = random(3000);
        // modbusData[3] = random(7500);
        modbusData[4] = random(8);

        // === SYSTEM LOG DATA ===
        uint32_t freeHeap = ESP.getFreeHeap();
        modbusData[5] = (uint16_t)(freeHeap >> 16);    // High word
        modbusData[6] = (uint16_t)(freeHeap & 0xFFFF); // Low word
        uint32_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        modbusData[7] = (uint16_t)(largestBlock >> 16);
        modbusData[8] = (uint16_t)(largestBlock & 0xFFFF);
        uint32_t minHeap = ESP.getMinFreeHeap();
        modbusData[9] = (uint16_t)(minHeap >> 16);
        modbusData[10] = (uint16_t)(minHeap & 0xFFFF);
        modbusData[11] = (uint16_t)esp_reset_reason();

        // === APP CONFIG DATA ===
        uint32_t prevDelay = delayReading;
        delayReading = prevDelay == modbusData[12] ? prevDelay : modbusData[12];
        modbusData[12] = delayReading;
        modbusData[13] = shouldRestartNow;
        modbusData[14] = shouldResetRainfall;
        modbusData[15] = fabs(WiFi.RSSI());

        delayReading = modbusData[12];

        if (shouldRestartNow)
            ESP.restart();

        if (shouldResetRainfall)
            counterRainfall = 0;

        struct tm timeinfo;
        getLocalTime(&timeinfo);
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
        if (hour == 23 && minute == 59 && !hasResetToday)
        {
            counterRainfall = 0;
            hasResetToday = true;
            Serial.println("Counter reset at 23:59 PM");
        }
        if (hour == 0 || minute == 0)
            hasResetToday = false;
        counterRainfall = 0; // Reset each cycle send

        // TODO: (OPTIONAL) STORE COUNTER AT EEPROM IN CASE OF WATCHDOG / RESET
    }
}

void ARDUINO_ISR_ATTR rainfallInterruptHandler()
{
    if (millis() - prevDebounceRainfall > debounceRainfall)
    {
        prevDebounceRainfall = millis();
        counterRainfall++;
    }
}

void ARDUINO_ISR_ATTR anemoInterruptHandler()
{
    if (millis() - prevDebounceAnemo > debounceAnemo)
    {
        prevDebounceAnemo = millis();
        counterAnemo++;
    }
}

ModbusMessage
FC03(ModbusMessage request)
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

        Serial.printf("Req Slave Id: %d, FC: %d, Data: [%d, %d, %d, %d, %d]\n",
                      request.getServerID(), request.getFunctionCode(),
                      modbusData[addr + 0], modbusData[addr + 1], modbusData[addr + 2], modbusData[addr + 3], modbusData[addr + 4]);

        WebSerial.printf("Req Slave Id: %d, FC: %d, Data: [%d, %d, %d, %d, %d]\n",
                         request.getServerID(), request.getFunctionCode(),
                         modbusData[addr + 0], modbusData[addr + 1], modbusData[addr + 2], modbusData[addr + 3], modbusData[addr + 4]);
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

    // Address overflow check
    if (addr >= 16)
    { // Your modbusData array is 16 words
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        return response;
    }

    // Write to modbusDatary
    modbusData[addr] = value;

    // Echo back the request (standard FC06 response)
    response.add(request.getServerID(), request.getFunctionCode());
    response.add(addr);
    response.add(value);

    return response;
}