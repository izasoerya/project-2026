#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <ClosedCube_SHT31D.h>
#include <WireGuard-ESP32.h>

#include "config.h" // .env
#include "models.h"
#include "../utils/parser.h"
#include "transmitter/configs/wifi_module.h"
#include "../utils/utils.h"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "slave-bandung-persemaian-2";
WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);

AsyncWebServer server(80);
WireGuard wg;

ModbusServerTCPasync modbusServer;
const uint8_t MAX_REGISTER = 16;
uint16_t modbusData[MAX_REGISTER];
ModbusMessage FC03(ModbusMessage request);
ModbusMessage FC06(ModbusMessage request);

const uint8_t pinAnemo = 9;
const uint8_t pinRainfall = 10;
const uint8_t pinWindDirectionRX = 4; //! 4 FOR CIMINYAK, 5 FOR CISANGKUY
const uint8_t pinWindDirectionTX = 3; //! 3 FOR CIMINYAK, 6 FOR CISANGKUY
const uint8_t pinSDA = 7;
const uint8_t pinSCL = 8;

ClosedCube_SHT31D sht3xd;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

volatile uint32_t counterAnemo = 0;
const uint16_t debounceAnemo = 5; // milisecond
uint32_t prevDebounceAnemo = 0;

volatile uint32_t counterRainfall = 0;
const uint16_t debounceRainfall = 500; // milisecond
uint32_t prevDebounceRainfall = 0;

uint32_t prevTimeReading = 0;
uint32_t prevWeatherReading = 0;
uint32_t delayReading = 10000;
const uint32_t weatherReadingInterval = 30000;
bool shouldRestartNow = false;
bool shouldResetRainfall = false;
bool hasResetToday = false;

void ARDUINO_ISR_ATTR rainfallInterruptHandler();
void ARDUINO_ISR_ATTR anemoInterruptHandler();

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting data acquization!");
    Serial1.begin(9600, SERIAL_8N1, pinWindDirectionRX, pinWindDirectionTX);

    if (wifi.begin())
        Serial.println(wifi.localIP());

    esp_task_wdt_init(60, true);
    esp_task_wdt_add(NULL);

    pinMode(pinAnemo, INPUT_PULLUP);
    pinMode(pinRainfall, INPUT_PULLUP);

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server);
    ElegantOTA.onEnd([](bool success)
                     { if(success) esp_restart(); });
    WebSerial.begin(&server);
    server.begin();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Test Successful!"); });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
              { esp_restart(); });

    Wire.begin(pinSDA, pinSCL);
    sht3xd.begin(0x44); // I2C address can be 0x44 or 0x45
    Serial.print("SHT3X serial #: ");
    Serial.println(sht3xd.readSerialNumber());

    if (sht3xd.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) != SHT3XD_NO_ERROR)
    {
        Serial.println("SHT3X periodic mode failed");
        WebSerial.println("SHT3X periodic mode failed");
    }

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    detachInterrupt(pinAnemo);
    detachInterrupt(pinRainfall);
    attachInterrupt(digitalPinToInterrupt(pinAnemo), anemoInterruptHandler, FALLING);
    attachInterrupt(digitalPinToInterrupt(pinRainfall), rainfallInterruptHandler, FALLING);

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
    wgLocalIP.fromString(WG_DEVICE_SLAVE_LOCAL_IP_2);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_SLAVE_PRIVATE_KEY_2,
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

bool counterAnemoResetThisCycle = false;
uint32_t prevDir = 0;

void loop()
{
    esp_task_wdt_reset();

    if (millis() - prevTimeReading > delayReading)
    {
        Serial.println("HEARTBEAT");
        WebSerial.println("HEARTBEAT");
        prevTimeReading = millis();

        // === SENSOR DATA ===
        if (millis() - prevWeatherReading > weatherReadingInterval)
        {
            prevWeatherReading = millis();

            {
                String data = Serial1.readString(); // data yang diterima dari sensor berawalan tanda * dan diakhiri tanda #, contoh *1#
                int a = data.indexOf("*");          // a adalah index tanda *
                int b = data.indexOf("#");          // b adalah index tanda #
                String resultWind = data.substring(a + 1, b);
                modbusData[4] = Parser::parseStringWindDirection(resultWind);
            }
            // uint16_t windDirectionRegister = modbusData[4];
            // if (fetchOpenMeteoWindDirection(windDirectionRegister))
            // {
            //     modbusData[4] = windDirectionRegister;
            //     Serial.printf("Open-Meteo wind direction register: %u\n", modbusData[4]);
            //     WebSerial.printf("Open-Meteo wind direction register: %u\n", modbusData[4]);
            // }
        }

        SHT31D shtResult = sht3xd.periodicFetchData();
        if (shtResult.error == SHT3XD_NO_ERROR)
        {
            modbusData[0] = Utils::toDeciU16(shtResult.t);  // temperature in 0.1 C
            modbusData[1] = Utils::toDeciU16(shtResult.rh); // humidity in 0.1 %RH
        }
        else
        {
            Serial.printf("SHT3X read error: %d\n", shtResult.error);
            WebSerial.printf("SHT3X read error: %d\n", shtResult.error);
        }

        float rainFallResult = counterRainfall * 0.7;                                   // Return in mm/<time>
        float rpm = float(counterAnemo / (delayReading / 1000.0));                      // Return in rotation/minute
        float anemoResult = ((-0.0181 * (rpm * rpm))) + (1.3859 * float(rpm)) + 1.4055; // Return in m/s
        modbusData[2] = uint16_t(rainFallResult * 10);                                  // Store as uint and .1 precision
        if (anemoResult < 1.5)
            modbusData[3] = 0;
        else
            modbusData[3] = uint16_t(anemoResult * 10); // Store as uint and .1 precision
        counterAnemoResetThisCycle = false;

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
            esp_restart();

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

        Serial.printf("Req Slave Id: %d, FC: %d, Data: [%d, %d, %d, %d, %d]\n",
                      request.getServerID(), request.getFunctionCode(),
                      modbusData[addr + 0], modbusData[addr + 1], modbusData[addr + 2], modbusData[addr + 3], modbusData[addr + 4]);

        WebSerial.printf("Req Slave Id: %d, FC: %d, Data: [%d, %d, %d, %d, %d]\n",
                         request.getServerID(), request.getFunctionCode(),
                         modbusData[addr + 0], modbusData[addr + 1], modbusData[addr + 2], modbusData[addr + 3], modbusData[addr + 4]);
    }
    if (!counterAnemoResetThisCycle)
    {
        counterAnemo = 0;
        counterAnemoResetThisCycle = true;
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

//=============================================== INTERRUPT TEST =======================================================
// #include <Arduino.h>
// #define ANEMO_PIN 9           // Change to your actual GPIO
// #define RAIN_PIN 10 // Change to your actual GPIO
// volatile uint32_t anemoPulseCount = 0;
// volatile uint32_t rainPulseCount = 0;
// uint32_t prevAnemoCount = 0;
// void IRAM_ATTR anemoISR()
// {
//     if (millis() - prevAnemoCount > 500)
//     {
//         prevAnemoCount = millis();
//         anemoPulseCount++;
//     }
// }
// uint32_t prevRainCount = 0;
// void IRAM_ATTR rainISR()
// {
//     if (millis() - prevRainCount > 100)
//     {
//         prevRainCount = millis();
//         rainPulseCount++;
//     }
// }
// void setup()
// {
//     Serial.begin(115200);
//     pinMode(ANEMO_PIN, INPUT_PULLUP); // Most anemometers are open-collector/reed switch
//     pinMode(RAIN_PIN, INPUT_PULLUP);  // Most anemometers are open-collector/reed switch
//     gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
//     detachInterrupt(ANEMO_PIN);
//     detachInterrupt(RAIN_PIN);
//     attachInterrupt(
//         digitalPinToInterrupt(ANEMO_PIN),
//         anemoISR,
//         FALLING // Try RISING or CHANGE if needed
//     );
//     attachInterrupt(
//         digitalPinToInterrupt(RAIN_PIN),
//         rainISR,
//         FALLING // Try RISING or CHANGE if needed
//     );
//     Serial.println("Interrupt test started");
// }
// void loop()
// {
//     static uint32_t lastPrint = 0;
//     if (millis() - lastPrint >= 1000)
//     {
//         detachInterrupt(RAIN_PIN);
//         lastPrint = millis();
//         noInterrupts();
//         uint32_t anemoCount = anemoPulseCount;
//         uint32_t rainCount = rainPulseCount;
//         interrupts();
//         Serial.printf("RAIN_PIN state: %d\n", digitalRead(RAIN_PIN));
//         // Serial.printf("anemo: %lu | rain: %lu\n", anemoCount, rainCount);
//     }
// }

// =============================================== I2C SCAN TEST =======================================================
// #include <Arduino.h>
// #include <Wire.h>
// void setup()
//{
//    Wire.begin(7, 8);
//    Serial.begin(115200);
//    Serial.println("\nI2C Scanner");
//}
// void loop()
//{
//    byte error, address;
//    int nDevices;
//    Serial.println("Scanning...");
//    nDevices = 0;
//    for (address = 1; address < 127; address++)
//    {
//        Wire.beginTransmission(address);
//        error = Wire.endTransmission();
//        if (error == 0)
//        {
//            Serial.print("I2C device found at address 0x");
//            if (address < 16)
//            {
//                Serial.print("0");
//            }
//            Serial.println(address, HEX);
//            nDevices++;
//        }
//        else if (error == 4)
//        {
//            Serial.print("Unknow error at address 0x");
//            if (address < 16)
//            {
//                Serial.print("0");
//            }
//            Serial.println(address, HEX);
//        }
//    }
//    if (nDevices == 0)
//    {
//        Serial.println("No I2C devices found\n");
//    }
//    else
//    {
//        Serial.println("done\n");
//    }
//    delay(5000);
//}

// ================================== SERIAL TEST ==========================================

// #include <Arduino.h>

// void setup()
// {
//     Serial.begin(115200);
//     Serial1.begin(9600, SERIAL_8N1, 4, 3); // RX=5, TX=6
//     Serial.println("Starting Serial1 loopback test...");
// }

// void loop()
// {

//     if (Serial1.available())
//     {
//         String data = Serial1.readString();
//         Serial.print("Serial1 received: ");
//         Serial.println(data);
//     }
//     else
//     {
//         Serial.println("Serial1 available: NO");
//     }
//     delay(1000);
// }