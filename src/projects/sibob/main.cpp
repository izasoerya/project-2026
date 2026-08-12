#include <Arduino.h>
#include <WiFi.h>
#include <ElegantOTA.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <HX711.h>
#include <WireGuard-ESP32.h>

#include "esp_task_wdt.h"
#include "transmitter/configs/wifi_module.h"
#include "sensor/configs/dht_sensor.h"
#include "sensor/configs/ads_sensor.h"
#include "sensor/configs/ds18b20_sensor.h"

#include "bang-bang.h"
#include "models.h"
#include "config.h"
#include "hx711_reading.h"

// #define SIBOB_1
#define SIBOB_2

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

#define CHANNEL_PH 0
#define CHANNEL_SOIL_HUM 1

#define TOP_TEMP_SET 25
#define BOT_TEMP_SET 10
#define TOP_HUM_SET 90
#define BOT_HUM_SET 40

const char *ssid = "Bengkel Inovasi Indonesia";
const char *password = "EKSPEKTASI";
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
ADSSensor phSensor(1, "PH", CHANNEL_PH, &ads, [](float v)
                   { return v; });
ADSSensor soilHum(1, "Soil Humidity", CHANNEL_SOIL_HUM, &ads, [](float v)
                  { return v; });
HX711 hx1;
HX711 hx2;
DS18B20Sensor ds(1, "WATER TEMP", PIN_DS18);

SensorData sensors;

int publishSensorSnapshot()
{
    JsonDocument supabaseDoc;
    supabaseDoc["temperature_soil"] = sensors.temperature_soil.value;
    supabaseDoc["humidity_soil"] = sensors.humidity_soil.value;
    supabaseDoc["ph"] = sensors.ph.value;
    supabaseDoc["weight"] = (sensors.weight_breed.value + sensors.weight_yield.value) / 2.0;
    supabaseDoc["temperature_air"] = sensors.temperature_air.value;
    supabaseDoc["humidity_air"] = sensors.humidity_air.value;
    supabaseDoc["device_id"] = sensors.id;

    String payload;
    int bytes = serializeJson(supabaseDoc, payload);

    Serial.printf("Payload length: %d bytes\n", payload.length());
    Serial.printf("Serialized: %d bytes\n", bytes);
    for (int i = 0; i < payload.length() && i < 100; i++)
    {
        Serial.printf("%02X ", (uint8_t)payload[i]);
    }
    Serial.println();
    Serial.printf("PAYLOAD: %s\n", payload.c_str());

    return inet.send("%5BSIBOB%5D%20sensor", payload.c_str());
}

void setup()
{
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);

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
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_PRIVATE_KEY_1,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
#endif // SIBOB_1

#if defined(SIBOB_2)
    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_LOCAL_IP_2);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
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
    sensors.weight_breed.status = weightBegin(hx1, PIN_DT_BREED, PIN_CLK);
    sensors.weight_yield.status = weightBegin(hx2, PIN_DT_YIELD, PIN_CLK);
    sensors.temperature_soil.status = ds.begin();
    sensors.ph.status = ads.begin(ADS1115_PGA_2P048); // It's just ads

    // same sensor with temp air
    sensors.humidity_air.status = sensors.temperature_air.status;
    // same adc with ph
    sensors.humidity_soil.status = sensors.ph.status;
}

uint32_t lastUpdate = 0;
uint32_t lastLogLocal = 0;
uint32_t lastTimerHeater = 0;

void loop()
{
    inet.reconnect();
    esp_task_wdt_reset();

    static const BangBangController bang(
        BangBangConfig{TOP_TEMP_SET, BOT_TEMP_SET, // top temp, bot temp
                       TOP_HUM_SET, BOT_HUM_SET},  // top hum, bot hum
        ActuatorConfig{0, 1, 3});                  // pin fan, pin mist, pin heater

    if (millis() - lastLogLocal >= 5000)
    {
#if defined(SIBOB_1)
        sensors.id = 1;
        sensors.weight_breed.value = weightReading(hx1);
        sensors.weight_yield.value = weightReading(hx2);
#elif defined(SIBOB_2)
        sensors.id = 2;
        sensors.weight_breed.value = weightReading(hx1);
        sensors.weight_yield.value = weightReading(hx2);
#endif
        WebSerial.printf("Weight: %.1f\n",
                         (sensors.weight_breed.value * 0.5 + sensors.weight_yield.value * 0.5));

        sensors.temperature_air.value = dhtSensor.getTemperature();
        sensors.humidity_air.value = dhtSensor.getHumidity();
        sensors.temperature_soil.value = ds.read();
        sensors.humidity_soil.value = soilHum.read();
        sensors.ph.value = phSensor.read();

        String buffer;
        serializeJsonPretty(sensors.toJsonDocument(), buffer);
        Serial.println(buffer);

        lastLogLocal = millis();
    }

    if (millis() - lastUpdate >= 20000)
    {
#if defined(SIBOB_1)
        sensors.id = 1;
        sensors.weight_breed.value = weightReading(hx1);
        sensors.weight_yield.value = weightReading(hx2);
#elif defined(SIBOB_2)
        sensors.id = 2;
        sensors.weight_breed.value = weightReading(hx1);
        sensors.weight_yield.value = weightReading(hx2);
#endif
        WebSerial.printf("Weight: %.1f\n",
                         (sensors.weight_breed.value * 0.5 + sensors.weight_yield.value * 0.5));

        sensors.temperature_air.value = dhtSensor.getTemperature();
        sensors.humidity_air.value = dhtSensor.getHumidity();
        sensors.temperature_soil.value = ds.read();
        sensors.humidity_soil.value = soilHum.read();
        sensors.ph.value = phSensor.read();

        int response = publishSensorSnapshot();
        Serial.printf("Supabase Res Code: %d\n", response);
        WebSerial.printf("Supabase Res Code: %d\n", response);
        lastUpdate = millis();
    }

    if (millis() - lastTimerHeater >= 10000)
    {
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

    ElegantOTA.loop();
}