#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <LiquidCrystal_I2C.h>
#include "display/lcd_i2c_basic.h"
#include <WireGuard-ESP32.h>

#include "../utils/utils.h"
#include "../utils/parser.h"
#include "models.h"
#include "config.h"

#include "reader-module/ads1115_module.h"
#include "sensor/configs/ds18b20_sensor.h"
#include "sensor/configs/bh1750_sensor.h"
#include "sensor/configs/ultrasonic_sensor.h"
#include "sensor/configs/ph_ph4502c.h"
#include "sensor/configs/tds_dfrobot_sensor.h"
#include "sensor/filters/moving_average.h"

#include "transmitter/configs/lora.h"
#include "transmitter/configs/wifi_module.h"

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6MXdBYHV3"
#define BLYNK_TEMPLATE_NAME "Aquaponic"
#include "transmitter/configs/wifi_blynk.h"

#define BLYNK_AMBIENT_LIGHT_PIN V0
#define BLYNK_WATER_LEVEL_PIN V1
#define BLYNK_WATER_PH_PIN V2
#define BLYNK_WATER_TEMPERATURE_PIN V3
#define BLYNK_TDS_PIN V4
#define BLYNK_WATER_PUMP_PIN V5
#define BLYNK_LED_PIN V6
#define BLYNK_AIR_PUMP_PIN V7

#define PIN_ECHO 8
#define PIN_TRIG 7
#define PIN_DS18 4
#define PIN_SDA 5
#define PIN_SCL 6
#define PIN_WATER_PUMP 0
#define PIN_LED 1
#define PIN_AIR_PUMP 3

#define ADDRESS_BH1750 0x23
#define ADDRESS_ADS1115 0x48
#define ADDRESS_OLED 0x3C
#define ADS_CHANNEL_PH 0
#define ADS_CHANNEL_TDS 1

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostName = "aquaponic-1";
const char *blynkAuthToken = "d2oR-C4x_VlT26WNVydzEKntp-865JkX";
WiFiBlynk blynk(
    blynkAuthToken,
    ssid, password, hostName,
    wifi_power_t::WIFI_POWER_8_5dBm, // since im using ESP32 C3 Super Mini Black
    [](uint8_t virtualPin, bool state)
    {
        if (virtualPin == BLYNK_WATER_PUMP_PIN)
        {
            digitalWrite(PIN_WATER_PUMP, state);
            Serial.printf("Water Pump ON Pin %d, %d\n", PIN_WATER_PUMP, state);
        }
        else if (virtualPin == BLYNK_LED_PIN)
        {
            digitalWrite(PIN_LED, state);
            Serial.printf("LED ON Pin %d, %d\n", PIN_LED, state);
        }
        else if (virtualPin == BLYNK_AIR_PUMP_PIN)
        {
            digitalWrite(PIN_AIR_PUMP, state);
            Serial.printf("Air Pump ON Pin %d, %d\n", PIN_AIR_PUMP, state);
        }
    });
WireGuard wg;

ADS1115Module ads(ADS1115_DEFAULT_ADDRESS, &Wire);

DS18B20Sensor waterTemperatureSensor(
    1, "Water Temperature",
    PIN_DS18);

UltrasonicSensor waterLevelSensor(
    1, "Water Level",
    PIN_ECHO, PIN_TRIG);

BH1750Sensor lightIntensitySensor(
    1, "Light Intensity",
    &Wire, ADDRESS_BH1750);

TrimmedMovingAverage filterPH(20, 5);
PH4502CSensor phSensor(
    1, "PH DFRobot",
    ADS_CHANNEL_PH, &ads,
    &filterPH, &waterTemperatureSensor);

TrimmedMovingAverage filterTDS(20, 5);
TDSDFRobotSensor tdsSensor(
    1, "TDS DFRobot",
    ADS_CHANNEL_TDS, &ads,
    &filterTDS, &waterTemperatureSensor);

AsyncWebServer server(80);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

AppState state = AppState::NORMAL_MODE;
uint64_t prevBlynkSend = 0;
uint64_t prevSampling = 0;
uint8_t adsSensorCounter = 0;
uint64_t prevScreen = 0;
float arrayADS[2];

// #define CALIBRATION

void setup()
{
    Serial.begin(115200);
    if (!blynk.begin())
        Serial.println("WiFi is not connected, disabling OTA!");

    pinMode(PIN_WATER_PUMP, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_AIR_PUMP, OUTPUT);
    Wire.begin(PIN_SDA, PIN_SCL);

    lcd.init(); // initialize the lcd
    lcd.backlight();
    lcd.createChar(0, (uint8_t *)temperature_icon);
    lcd.createChar(1, (uint8_t *)humidity_icon);
    lcd.createChar(2, (uint8_t *)turbidity_icon);
    lcd.createChar(3, (uint8_t *)ph_icon);
    lcd.createChar(4, (uint8_t *)light_icon);
    lcd.createChar(5, (uint8_t *)level_icon);

    ads.begin(ADS1115_MV_4P096);
    tdsSensor.begin();
    phSensor.begin();
    waterTemperatureSensor.begin();
    waterLevelSensor.begin();
    lightIntensitySensor.begin();

    WebSerial.begin(&server);
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);

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
    wgLocalIP.fromString(WG_DEVICE_LOCAL_IP);
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_PRIVATE_KEY,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);

    ElegantOTA.begin(&server);
    server.begin();
}

#ifndef CALIBRATION

void loop()
{
    blynk.run();
    blynk.reconnect();
    ElegantOTA.loop();

    static AquaponicSensorEntity sensor;
    if (millis() - prevSampling > 50)
    {
        /**
         * @brief Reading in turn since ADS is multiplexer
         * if you read at once, i2c error like -1 or 263 will show up (from experiece)
         */
        if (adsSensorCounter == 0)
        {
            arrayADS[0] = tdsSensor.read();
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 1)
        {
            arrayADS[1] = phSensor.readIntercept(2.68, 3.15);
            adsSensorCounter = 0;
        }

        sensor.waterTDS = arrayADS[0];
        sensor.waterPH = arrayADS[1];
        sensor.lightIntensity = uint16_t(lightIntensitySensor.read()); // These are not using ads so its fine to poll
        sensor.waterTemperature = waterTemperatureSensor.read();       // These are not using ads so its fine to poll
        sensor.waterLevel = waterLevelSensor.read() - 35.0;
        if (sensor.waterLevel < 0)
            sensor.waterLevel = 0;

        prevSampling = millis();
    }

    if (millis() - prevBlynkSend > 30000)
    {
        const char *sensorString = sensor.toString();
        Serial.println(sensorString);
        WebSerial.println(sensorString);

        AquaponicSystemEntity system{
            .freeHeap = ESP.getFreeHeap(),
            .largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT),
            .minFreeHeap = ESP.getMinFreeHeap(),
            .lastResetReason = esp_reset_reason(),
        };
        const char *systemString = system.toString();
        Serial.println(systemString);
        WebSerial.println(sensorString);

        blynk.send(BLYNK_AMBIENT_LIGHT_PIN, sensor.lightIntensity);
        blynk.send(BLYNK_WATER_LEVEL_PIN, sensor.waterLevel);
        blynk.send(BLYNK_WATER_PH_PIN, sensor.waterPH);
        blynk.send(BLYNK_TDS_PIN, sensor.waterTDS);
        blynk.send(BLYNK_WATER_TEMPERATURE_PIN, sensor.waterTemperature);

        lcd.setCursor(0, 0);
        lcd.write(byte(0));
        lcd.printf("%.1fC", sensor.waterTemperature);

        lcd.setCursor(7, 0);
        lcd.write(byte(3));
        lcd.printf("%.1fPH", sensor.waterPH);

        lcd.setCursor(0, 1);
        lcd.write(byte(4));
        lcd.printf("%dLX", sensor.lightIntensity);

        lcd.setCursor(6, 1);
        lcd.write(byte(2));
        lcd.printf("%.1fppm", sensor.waterTDS);

        prevBlynkSend = millis();
    }
}

#endif

#ifdef CALIBRATION
int16_t array[4];
uint32_t prevCalibration = 0;
void loop()
{
    // blynk.run();
    // blynk.reconnect();
    ElegantOTA.loop();

    if (millis() - prevCalibration > 50)
    {
        if (adsSensorCounter == 0)
        {
            array[0] = ads.read(0);
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 1)
        {
            array[1] = phSensor.readRawVoltage();
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 2)
        {
            array[2] = ads.read(2);
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 3)
        {
            array[3] = ads.read(3);
            adsSensorCounter = 0;
        }
        WebSerial.printf("A0: %d | A1: %d | A2: %d | A3 %d\n",
                         array[0], array[1], array[2], array[3]);

        prevCalibration = millis();
    }
}
#endif
