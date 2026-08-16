#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <LiquidCrystal_I2C.h>
#include <INA219.h>
#include <WireGuard-ESP32.h>

#include "display/lcd_i2c_basic.h"

#include "../utils/utils.h"
#include "../utils/parser.h"
#include "models.h"
#include "config.h"

#include "reader-module/ads1115_module.h"
#include "sensor/configs/ph_ph4502c.h"
#include "sensor/filters/moving_average.h"
#include "sensor/configs/tds_dfrobot_sensor.h"
#include "sensor/configs/ph_dfrobot_sensor.h"

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL69LwuKF9Y"
#define BLYNK_TEMPLATE_NAME "pikohidro"
#include "transmitter/configs/wifi_blynk.h"

#define BLYNK_WATER_PH_PIN V1
#define BLYNK_TDS_PIN V0
#define BLYNK_TURBIDITY_PIN V2
#define BLYNK_POWER_IN_PIN V3
#define BLYNK_POWER_OUT_PIN V4

#define PIN_SDA 8
#define PIN_SCL 9

#define ADDRESS_ADS1115 0x48
#define ADDRESS_OLED 0x3C
#define ADS_CHANNEL_TURBIDITY 1
#define ADS_CHANNEL_PH 0
#define ADS_CHANNEL_TDS 2

const uint16_t adsResolution = 32768;
const float adsRef = 4.096;

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostName = "pikohidro-1";
const char *blynkAuthToken = "n6wt8wvIYJ6AoXrkmXtvI_8C0ATjMSmt";
WiFiBlynk blynk(
    blynkAuthToken,
    ssid,
    password,
    hostName,
    wifi_power_t::WIFI_POWER_8_5dBm,
    [](uint8_t virtualPin, bool state) {});
WireGuard wg;

ADS1115Module ads(ADDRESS_ADS1115, &Wire);

ADSSensor turbiditySensor(
    1, "Turbidity Sensor",
    ADS_CHANNEL_TURBIDITY, &ads,
    [](float value) -> float
    {
        float ntu = (-1120.4 * pow(value, 2)) + (5742.3 * value) - 4352.9;
        return ntu < 0 ? 0.0f : ntu;
    });

TrimmedMovingAverage filterPH(40, 10);
PH4502CSensor phSensor(
    1, "PH Sensor",
    ADS_CHANNEL_PH, &ads,
    &filterPH);

TrimmedMovingAverage filterTDS(20, 5);
TDSDFRobotSensor tdsSensor(
    1, "TDS Analog",
    ADS_CHANNEL_TDS, &ads,
    &filterTDS);

INA219 inaInput(0x40);
INA219 inaOutput(0x41);

LiquidCrystal_I2C lcd(0x27, 20, 4);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

AppState state = AppState::NORMAL_MODE;
uint8_t adsSensorCounter = 0;
AsyncWebServer server(80);
float arrayADS[3];

// #define CALIBRATION

void setup()
{
    Serial.begin(115200);
    if (!blynk.begin())
        Serial.println("WiFi is not connected, disabling OTA!");

    Wire.begin(PIN_SDA, PIN_SCL);

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

    lcd.init(); // initialize the lcd
    lcd.backlight();
    lcd.createChar(0, (uint8_t *)temperature_icon);
    lcd.createChar(1, (uint8_t *)ph_icon);
    lcd.createChar(2, (uint8_t *)tds_icon);
    lcd.createChar(3, (uint8_t *)turbidity_icon);
    lcd.createChar(4, (uint8_t *)wifi_icon);
    lcd.createChar(5, (uint8_t *)power_icon);

    ads.begin(ADS1115_MV_4P096);
    tdsSensor.begin();
    phSensor.begin();

    inaInput.begin();
    inaInput.setMaxCurrentShunt(5, 0.001);
    inaOutput.begin();
    inaOutput.setMaxCurrentShunt(5, 0.001);

    WebSerial.begin(&server);
    WebSerial.onMessage(
        [&](uint8_t *data, size_t len)
        {
            String req;
            req.reserve(len + 1);
            for (size_t i = 0; i < len; i++)
                req += (char)data[i];
            req.trim();
            if (req == "DEBUG_ADS")
                state = AppState::ENABLE_LOGGING_ADS;
            else if (req == "NORMAL")
                state = AppState::NORMAL_MODE;
        });

    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);

    server.begin();
}

#ifndef CALIBRATION

uint32_t prevSendBlynk = 0;
uint32_t prevSampling = 0;
uint32_t prevSensorLog = 0;
uint32_t prevLCDLog = 0;

void loop()
{
    blynk.run();
    blynk.reconnect();
    ElegantOTA.loop();

    static PikohidroSensorEntity sensor;
    if (millis() - prevSampling > 50)
    {
        /**
         * @brief Reading in turn since ADS is multiplexer
         * if you read at once, i2c error like -1 or 263 will show up (from experiece)
         */
        if (adsSensorCounter == 0)
        {
            arrayADS[0] = turbiditySensor.read();
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 1)
        {
            arrayADS[1] = phSensor.readIntercept(2.525, 2.92);
            adsSensorCounter++;
        }
        else if (adsSensorCounter == 2)
        {
            arrayADS[2] = tdsSensor.read();
            adsSensorCounter = 0;
        }
        sensor.waterTurbidity = arrayADS[0];
        sensor.waterPH = arrayADS[1];
        sensor.waterTDS = arrayADS[2];
        sensor.powerIn = inaInput.getPower();   // These are not using ads so its fine to poll
        sensor.powerOut = inaOutput.getPower(); // These are not using ads so its fine to poll

        prevSampling = millis();
    }

    if (millis() - prevSensorLog > 250 && state == AppState::ENABLE_LOGGING_ADS)
    {
        prevSensorLog = millis();
        Serial.printf("TDS: %d | PH: %d | Turb: %d\n",
                      tdsSensor.readRawVoltage(), phSensor.readRawVoltage(), turbiditySensor.readRawVoltage());
        WebSerial.printf("TDS: %d | PH: %d | Turb: %d\n",
                         tdsSensor.readRawVoltage(), phSensor.readRawVoltage(), turbiditySensor.readRawVoltage());
    }

    if (millis() - prevLCDLog > 3000)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("===== PIKOHIDRO =====");

        lcd.setCursor(0, 1);
        lcd.write(byte(2));
        lcd.printf("%.1fPPM", abs(sensor.waterTDS));

        lcd.setCursor(10, 1);
        lcd.write(byte(1));
        lcd.printf("%.2fPH", abs(sensor.waterPH));

        lcd.setCursor(0, 2);
        lcd.write(byte(3));
        lcd.printf("%.1fNTU", abs(sensor.waterTurbidity));

        lcd.setCursor(10, 2);
        lcd.write(byte(4));
        lcd.printf("%ddBm", blynk.getSignalStrength());

        lcd.setCursor(0, 3);
        lcd.write(byte(5));
        lcd.printf("I:%.1fW", abs(sensor.powerIn));

        lcd.setCursor(10, 3);
        lcd.write(byte(5));
        lcd.printf("O:%.1fW", abs(sensor.powerOut));

        prevLCDLog = millis();
    }

    if (millis() - prevSendBlynk > 30000)
    {
        const char *sensorString = sensor.toString();
        Serial.println(sensorString);
        WebSerial.println(sensorString);

        PikohidroSystemEntity system{
            .freeHeap = ESP.getFreeHeap(),
            .largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT),
            .minFreeHeap = ESP.getMinFreeHeap(),
            .lastResetReason = esp_reset_reason(),
        };
        const char *systemString = system.toString();
        Serial.println(systemString);
        WebSerial.println(systemString);

        blynk.send(BLYNK_TURBIDITY_PIN, sensor.waterTurbidity);
        blynk.send(BLYNK_WATER_PH_PIN, sensor.waterPH);
        blynk.send(BLYNK_TDS_PIN, sensor.waterTDS);
        blynk.send(BLYNK_POWER_IN_PIN, sensor.powerIn);
        blynk.send(BLYNK_POWER_OUT_PIN, sensor.powerOut);

        prevSendBlynk = millis();
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
