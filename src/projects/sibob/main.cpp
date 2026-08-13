#include <Arduino.h>
#include <WiFi.h>
#include <ElegantOTA.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <WireGuard-ESP32.h>
#include "esp_task_wdt.h"

#include "transmitter/configs/wifi_module.h"
#include "sensor/configs/dht_sensor.h"
#include "sensor/configs/ads_sensor.h"
#include "sensor/configs/ds18b20_sensor.h"
#include "sensor/configs/hx711_sensor.h"
#include "sensor/filters/moving_average.h"

#include "bang-bang.h"
#include "models.h"
#include "config.h"

/**
 * @brief Device Configuration
 *
 * Uncomment the devivce that will be build
 */
#define SIBOB_1
// #define SIBOB_2

/**
 * @brief Pinout Configuration
 *
 * Change to appropriate pin
 */
#define PIN_FAN 0
#define PIN_MIST 1
#define PIN_DHT 2
#define PIN_HEATER 3
#define PIN_DT_BREED 4
#define PIN_DT_YIELD 5
#define PIN_SDA 6
#define PIN_SCL 7
#define PIN_EN_PH 8
#define PIN_DS18 9
#define PIN_CLK 10

/**
 * @brief ADS Channel Configuration
 *
 */
#define CHANNEL_PH 0
#define CHANNEL_SOIL_HUM 1

/**
 * @brief Pinout Configuration
 *
 * Change setpoint of bang-bang control
 * @param TOP_TEMP_SET -> set top cap temperature for fan
 * @param BOT_TEMP_SET -> set bot cap temperature for fan
 * @param TOP_HUM_SET -> set top cap humidity for mist
 * @param BOT_HUM_SET -> set bot cap humiidty for mist
 */
#define TOP_TEMP_SET 25
#define BOT_TEMP_SET 10
#define TOP_HUM_SET 90
#define BOT_HUM_SET 40

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
#if defined(SIBOB_1)
const char *hostname = "sibob-1";
#endif // SIBOB_1
#if defined(SIBOB_2)
const char *hostname = "sibob-2";
#endif // SIBOB_2
const char *supabaseUrl = "https://gothabjdasaphwzrjnto.supabase.co";
const char *supabasePublicKey = "sb_publishable_Dx3vXSh8qdQhM1Zi_V1MTQ_ifqdbX1o";
SupabaseTransport transport = SupabaseTransport(supabaseUrl, supabasePublicKey);

AsyncWebServer server(80);
WiFiModule inet(ssid, password, hostname, WIFI_POWER_19_5dBm);
WireGuard wg;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

ADS1115Module ads(ADS1115_DEFAULT_ADDRESS, &Wire);

DHTSensor dhtSensor(PIN_DHT);

/**
 * @brief PH Sensor Configuration
 *
 * PH is read using ads and return the value in adc-16 bit.
 * Trimmed moving average is implemented in this sensor. To disable directly return v.
 */
TrimmedMovingAverage phFilter(20, 2);
ADSSensor phSensor(1, "PH Soil", CHANNEL_PH,
                   &ads, [](float v) -> float
                   {  phFilter.filter(v); 
                    return v; }); // Intercept with formula here

/**
 * @brief Soil Humidity Sensor Configuration
 *
 * Soil Humidity is read using ads and return the value in adc-16 bit.
 * Trimmed moving average is implemented in this sensor. To disable directly return v.
 */
TrimmedMovingAverage soilHumFilter(20, 2);
ADSSensor soilHum(1, "Soil Humidity", CHANNEL_SOIL_HUM,
                  &ads, [](float v) -> float
                  { soilHumFilter.filter(v); 
                    return v; }); // Intercept with formula here

/**
 * @brief Weight Sensor 1 Configuration
 *
 * Weight Sensor is read, change the interceptor formula for calibrate.
 */
TrimmedMovingAverage weight1Filter(20, 2);
HX711Sensor hx1(1, "Weight 1",
                PIN_DT_BREED, PIN_CLK,
                [](float v) -> float
                { 
                    weight1Filter.filter(v);
                    return v; }); // Intercept with formula here

/**
 * @brief Weight Sensor 2 Configuration
 *
 * Weight Sensor is read, change the interceptor formula for calibrate.
 */
TrimmedMovingAverage weight2Filter(20, 2);
HX711Sensor hx2(1, "Weight 2",
                PIN_DT_YIELD, PIN_CLK,
                [](float v) -> float
                { 
                    weight2Filter.filter(v);
                    return v; }); // Intercept with formula here

DS18B20Sensor ds(1, "WATER TEMP", PIN_DS18);

static SensorData sensors;
static bool isCalibrationADS = false;
static bool isCalibrationHX711 = false;
static bool isTransmitSupabase = true;

void setup()
{
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);

    analogWriteFrequency(500);

    Serial.begin(115200);
    inet.begin(
        []() -> void
        { Serial.print("."); },
        []() -> void
        { esp_restart(); });
    inet.setTransport(&transport);

    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");
    WebSerial.onMessage(
        [&](uint8_t *data, size_t len)
        {
            String d = "";
            for (size_t i = 0; i < len; i++)
                d += char(data[i]);
            if (d == "ADS_CALIBRATE")
                isCalibrationADS = true;
            else if (d == "HX711_CALIBRATE")
                isCalibrationHX711 = true;
            else if (d == "DISABLE_SUPABASE")
                isTransmitSupabase = false;
            else if (d == "ENABLE_SUPABASE")
                isTransmitSupabase = true;
            else if (d == "NORMAL")
            {
                isCalibrationHX711 = false;
                isCalibrationADS = false;
            }
            WebSerial.println(d);
        });

    Wire.begin(PIN_SDA, PIN_SCL);

    pinMode(PIN_FAN, OUTPUT);
    pinMode(PIN_MIST, OUTPUT);
    pinMode(PIN_HEATER, OUTPUT);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                String buffer;
                serializeJsonPretty(sensors.toJsonDocument(), buffer);
                request->send(200, "application/json", buffer); });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                request->send(200, "text/plain", "Restarting now...");
				  esp_restart(); });
    server.begin();

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

#if defined(SIBOB_1)
    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_LOCAL_IP_1);
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_PRIVATE_KEY_1,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
#endif // SIBOB_1

#if defined(SIBOB_2)
    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_LOCAL_IP_2);
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_PRIVATE_KEY_2,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
#endif // SIBOB_2

    if (wgOk)
    {
        Serial.printf("WG INIT SUCCESS: %s", wgLocalIP.toString());
        WebSerial.printf("WG INIT SUCCESS: %s", wgLocalIP.toString());
    }
    else
    {
        Serial.println("WG INIT FAILED!");
        WebSerial.println("WG INIT FAILED!");
    }

    sensors.temperature_air.status = dhtSensor.begin();
    sensors.weight_breed.status = hx1.begin();
    sensors.weight_yield.status = hx2.begin();
    sensors.temperature_soil.status = ds.begin();
    sensors.ph.status = ads.begin(ADS1115_PGA_2P048); // It's just ads

    // same sensor with temp air
    sensors.humidity_air.status = sensors.temperature_air.status;
    // same adc with ph
    sensors.humidity_soil.status = sensors.ph.status;
}

uint32_t lastUpdate = 0;
uint32_t lastLogLocal = 0;
uint32_t lastLogADSDebug = 0;
uint32_t lastLogHX711Debug = 0;
uint32_t lastTimerHeater = 0;

void loop()
{
    inet.reconnect();
    esp_task_wdt_reset();

    if (millis() - lastLogLocal >= 5000 && !(isCalibrationADS || isCalibrationHX711))
    {
        const float W1_ZERO = 206441;    // Baseline W1
        const float W2_ZERO = 419842;    // Baseline W2
        const float CAL_FACTOR = 195.88; // units/gram
        const float TARE = -17.74;
#if defined(SIBOB_1)
        sensors.id = 1;
        sensors.weight_breed.value = (((hx1.read() - W1_ZERO) + (hx2.read() - W2_ZERO)) / CAL_FACTOR) + TARE;
        sensors.weight_yield.value = hx2.read();
#elif defined(SIBOB_2)
        sensors.id = 2;
        sensors.weight_breed.value = (((hx1.read() - W1_ZERO) + (hx2.read() - W2_ZERO)) / CAL_FACTOR) + TARE;
        sensors.weight_yield.value = hx2.read();
#endif
        sensors.temperature_air.value = dhtSensor.getTemperature();
        sensors.humidity_air.value = dhtSensor.getHumidity();
        sensors.temperature_soil.value = ds.read();
        sensors.humidity_soil.value = soilHum.read();
        sensors.ph.value = phSensor.read();

        const char *logSensor = sensors.toJson();
        Serial.println(logSensor);
        WebSerial.println(logSensor);

        lastLogLocal = millis();
    }

    if (millis() - lastUpdate >= 60000 && isTransmitSupabase)
    {
#if defined(SIBOB_1)
        sensors.id = 1;
        sensors.weight_breed.value = hx1.read();
        sensors.weight_yield.value = hx2.read();
#elif defined(SIBOB_2)
        sensors.id = 2;
        sensors.weight_breed.value = hx1.read();
        sensors.weight_yield.value = hx2.read();
#endif
        sensors.temperature_air.value = dhtSensor.getTemperature();
        sensors.humidity_air.value = dhtSensor.getHumidity();
        sensors.temperature_soil.value = ds.read();
        sensors.humidity_soil.value = soilHum.read();
        sensors.ph.value = phSensor.read();

        const char *jsonString = sensors.toJson();
        int response = inet.send("%5BSIBOB%5D%20sensor", jsonString);
        Serial.printf("Supabase Res Code: %d\n", response);
        WebSerial.printf("Supabase Res Code: %d\n", response);

        lastUpdate = millis();
    }

    if (millis() - lastTimerHeater >= 10000)
    {
        static const BangBangController bang(
            BangBangConfig{TOP_TEMP_SET, BOT_TEMP_SET, // top temp, bot temp
                           TOP_HUM_SET, BOT_HUM_SET},  // top hum, bot hum
            ActuatorConfig{0, 1, 3});                  // pin fan, pin mist, pin heater

        static bool flipFlag = false;
        bang.control(
            sensors.temperature_soil.value,
            sensors.humidity_soil.value);

        bang.controlHeater(
            sensors.temperature_soil.value,
            sensors.humidity_soil.value,
            flipFlag);

        flipFlag = !flipFlag;
        lastTimerHeater = millis();
    }

    if (millis() - lastLogADSDebug >= 200 && isCalibrationADS)
    {
        volatile uint16_t cachedADS[4] = {0};
        cachedADS[0] = ads.read(0);
        cachedADS[1] = ads.read(1);
        cachedADS[2] = ads.read(2);
        cachedADS[3] = ads.read(3);

        Serial.printf("CH0: %d | CH1: %d | CH2: %d | CH3: %d\n",
                      cachedADS[0], cachedADS[1], cachedADS[2], cachedADS[3]);
        WebSerial.printf("CH0: %d | CH1: %d | CH2: %d | CH3: %d\n",
                         cachedADS[0], cachedADS[1], cachedADS[2], cachedADS[3]);
        lastLogADSDebug = millis();
    }

    if (millis() - lastLogHX711Debug >= 200 && isCalibrationHX711)
    {
        Serial.printf("W1: %.1f | W2: %.1f\n",
                      hx1.readRaw(), hx2.readRaw());
        WebSerial.printf("W1: %.1f | W2: %.1f\n",
                         hx1.readRaw(), hx2.readRaw());

        lastLogHX711Debug = millis();
    }

    ElegantOTA.loop();
}
