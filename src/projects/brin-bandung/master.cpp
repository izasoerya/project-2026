#include <TFT_eSPI.h>
#include <ModbusClientTCPasync.h>
#include <time.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <WireGuard-ESP32.h>

#include "config.h" // .env
#include "models.h"
#include "display/display_tft_spi_lcd/display_tft.h"
#include "transmitter/configs/wifi_module.h"

#define TFT_SCK_PIN 8
#define TFT_MISO_PIN 20
#define TFT_MOSI_PIN 9
#define TFT_CS_PIN 5

TFT_eSPI tft = TFT_eSPI();
DisplayTFT320X480PV display(tft);

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostName = "master-bandung-persemaian-1"; //! RECHECK THIS EVERYTIME COMPILE
WiFiModule wifi(ssid, password, hostName, WIFI_POWER_8_5dBm);

const char *supabaseUrl = "https://pykernnkhvnssplhzcvn.supabase.co";
const char *supabasePublicKey = "sb_publishable_coDPUa845ZtfYmoBWlZlgw_eH5vsCY7";
SupabaseTransport transport = SupabaseTransport(supabaseUrl, supabasePublicKey);

AsyncWebServer server(80);
WireGuard wg;

const char *ntpServer = "pool.ntp.org";
const uint16_t gmtOffset_sec = 25200;
const uint16_t daylightOffset_sec = 0;

const char *modbusSlaveUrl = "slave-bandung-persemaian-1";        //! RECHECK THIS EVERYTIME COMPILE
const char *modbusSlaveArrUrl = "slave-arr-bandung-persemaian-1"; //! RECHECK THIS EVERYTIME COMPILE
const uint16_t modbusSlavePort = 5000;
ModbusClientTCPasync *modbusClient = nullptr;
ModbusClientTCPasync *modbusArrClient = nullptr;
void onDataHandler(ModbusMessage response, uint32_t token);
void onErrorHandler(Error error, uint32_t token);
void onDataHandlerArr(ModbusMessage response, uint32_t token);
void onErrorHandlerArr(Error error, uint32_t token);

const uint32_t deviceId = 1; //! RECHECK THIS EVERYTIME COMPILE

uint32_t prevSamplingMillis = 0;
uint32_t prevSystemLoggingMillis = 0;
uint32_t prevNTPMillis = 0;
uint32_t prevTransmitMillis = 0;

uint32_t stampDataModbusCounter = 0;
uint32_t stampDataModbusArrCounter = 0;
uint8_t errorTransactionModbusCounter = 0;
uint8_t errorTransactionModbusArrCounter = 0;

uint16_t sensorDatas[5];
uint16_t rainfallGravityData;

#define RAINFALL_GRAVITY

void fetchOpenMeteoData();

void setup()
{
    Serial.begin(115200);

    if (wifi.begin(
            []()
            { Serial.print("."); },
            []()
            { esp_restart(); }))
        Serial.printf("Connected to: %s\n", wifi.localIP());

    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onEnd([](bool success)
                     {if(success) esp_restart(); });
    WebSerial.begin(&server);
    server.begin();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Test Successful!"); });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
              { esp_restart(); });

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
            ESP.restart(); // Critical: WireGuard handshake will fail without correct time
        }
        retryCounter++;
    }

#if defined(RAINFALL_GRAVITY)
    IPAddress modbusSlaveArrIP;
    IPAddress resolvedArr = wifi.resolveMDNS(modbusSlaveArrUrl);
    if (resolvedArr != IPAddress(0, 0, 0, 0))
        modbusSlaveArrIP = resolvedArr;
    else
    {
        Serial.println("Failed to expect tipping bucket gravity");    // Continue with cheap tipping bucket
        WebSerial.println("Failed to expect tipping bucket gravity"); // Continue with cheap tipping bucket
    }
#endif // RAINFALL_GRAVITY

    IPAddress modbusSlaveIP;
    IPAddress resolved = wifi.resolveMDNS(modbusSlaveUrl);
    if (resolved != IPAddress(0, 0, 0, 0))
        modbusSlaveIP = resolved;
    else
    {
        Serial.println("Failed to expect slave data");    // Continue with cheap tipping bucket
        WebSerial.println("Failed to expect slave data"); // Continue with cheap tipping bucket
    }

    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_MASTER_LOCAL_IP_1); //! RECHECK THIS EVERYTIME COMPILE
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());

    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_MASTER_PRIVATE_KEY_1, //! RECHECK THIS EVERYTIME COMPILE
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
    if (wgOk)
        Serial.println("WireGuard successfully initialized on ESP32!");
    else
        Serial.println("WireGuard initialization failed!");

#if defined(RAINFALL_GRAVITY)
    modbusArrClient = new ModbusClientTCPasync(modbusSlaveArrIP, modbusSlavePort);
    modbusArrClient->connect();
    modbusArrClient->onDataHandler(&onDataHandlerArr);
    modbusArrClient->onErrorHandler(&onErrorHandlerArr);
    modbusArrClient->setTimeout(10000);
    modbusArrClient->setIdleTimeout(60000);
#endif // RAINFALL_GRAVITY

    modbusClient = new ModbusClientTCPasync(modbusSlaveIP, modbusSlavePort);
    modbusClient->connect();
    modbusClient->onDataHandler(&onDataHandler);
    modbusClient->onErrorHandler(&onErrorHandler);
    modbusClient->setTimeout(10000);
    modbusClient->setIdleTimeout(60000);

    SPI.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
    display.begin();
    display.setHeaderTitle("BANDUNG-SEEDBED-1");
    display.setFooterText("v1.0.0");
    display.setContainer1("TEMPERATURE", "0 *C", DisplayColor::ORANGE, IconType::THERMO);
    display.setContainer2("HUMIDITY", "0 %RH", DisplayColor::BLUE, IconType::DROPLET);
    display.setContainer3("WIND SPEED", "0 m/s", DisplayColor::TEXT, IconType::WIND);
    display.setContainer4("WIND DIR", "North", DisplayColor::YELLOW, IconType::COMPASS);
    display.setContainer5("RAINFALL", "0 mm/day", DisplayColor::TEAL, IconType::RAIN);
    display.setContainer6("COMPANY", "T4T x ZTS", DisplayColor::GREEN, IconType::COMPANY);
    display.drawLayout(); // one full paint of shells/borders/icons/labels
}

void loop()
{
    ElegantOTA.loop();
    wifi.reconnect();

    if (millis() - prevNTPMillis > 1000)
    {
        prevNTPMillis = millis();

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("[ERROR] NTP Error");
            WebSerial.println("[ERROR] NTP Error");
        }
        char timeHour[3];
        char timeMinute[3];
        char timeSecond[3];
        strftime(timeHour, 3, "%H", &timeinfo);
        strftime(timeMinute, 3, "%M", &timeinfo);
        strftime(timeSecond, 3, "%S", &timeinfo);
        display.setClock(atoi(timeHour), atoi(timeMinute), atoi(timeSecond));

        display.setSignalStrength(wifi.getRssi());
        display.refresh();
    }

    if (millis() - prevSystemLoggingMillis > 60000 * 60) // Every 1 hour
    {
        prevSystemLoggingMillis = millis();

        SystemLogDto systemLog{
            .deviceId = deviceId,
            .freeHeap = ESP.getFreeHeap(),
            .largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT),
            .minFreeHeap = ESP.getMinFreeHeap(),
            .lastResetReason = esp_reset_reason(),
        };
        Serial.println(systemLog.toString());
        WebSerial.println(systemLog.toString());

        wifi.setTransport(&transport);
        char buffer[256];
        systemLog.toJson(buffer, sizeof(buffer));
        int16_t response = wifi.send("system_logs", buffer);
        if (response != 200 && response != 201)
        {
            Serial.printf("POST Failed: %d\n", response);
            WebSerial.printf("POST Failed: %d\n", response);
        }
    }

    if (millis() - prevSamplingMillis > 60000 * 1) //  Every 1 minute
    {
        prevSamplingMillis = millis();

#if defined(RAINFALL_GRAVITY)
        Error errArr = modbusArrClient->addRequest(
            (uint32_t)stampDataModbusArrCounter, // Token
            1, READ_HOLD_REGISTER, 0, 1);
        if (errArr != SUCCESS)
        {
            ModbusError e(errArr);
            Serial.printf("Error ARR creating request: %02X - %s\n", (int)e, (const char *)e);
            WebSerial.printf("Error ARR creating request: %02X - %s\n", (int)e, (const char *)e);
        }
        stampDataModbusArrCounter++;
#endif // RAINFALL_GRAVITY

        // Error err = modbusClient->addRequest(
        //     (uint32_t)stampDataModbusCounter, // Token
        //     1, READ_HOLD_REGISTER, 0, 5);
        // if (err != SUCCESS)
        // {
        //     ModbusError e(err);
        //     Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
        //     WebSerial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);

        // }
        // stampDataModbusCounter++;
        fetchOpenMeteoData();

#if !defined RAINFALL_GRAVITY
        SensorDto sensor{
            .deviceId = deviceId,
            .rainFall = float(sensorDatas[2] / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };
#endif

#if defined(RAINFALL_GRAVITY)
        SensorDto sensor{
            .deviceId = deviceId,
            .rainFall = float(rainfallGravityData / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };
#endif // RAINFALL_GRAVITY

        Serial.println(sensor.toString());
        WebSerial.println(sensor.toString());

        char buf[24]; // Follow max char in custom library
        snprintf(buf, sizeof(buf), "%.1f C", sensor.airTemperature);
        display.updateContainerValue(1, buf);

        snprintf(buf, sizeof(buf), "%.1f %RH", sensor.airHumidity);
        display.updateContainerValue(2, buf);

        snprintf(buf, sizeof(buf), "%.1f km/h", sensor.windSpeed);
        display.updateContainerValue(3, buf);

        snprintf(buf, sizeof(buf), "%s", Parser::parseWindDirection(sensor.windDirection));
        display.updateContainerValue(4, buf);

        snprintf(buf, sizeof(buf), "%.1f mm/day", sensor.rainFall);
        display.updateContainerValue(5, buf);
    }

    if (millis() - prevTransmitMillis > 60000 * 60) // Every one hour
    {
        prevTransmitMillis = millis();
#if !defined RAINFALL_GRAVITY
        SensorDto sensor{
            .deviceId = deviceId,
            .rainFall = float(sensorDatas[2] / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };
#endif

#if defined(RAINFALL_GRAVITY)
        SensorDto sensor{
            .deviceId = deviceId,
            .rainFall = float(rainfallGravityData / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };
#endif // RAINFALL_GRAVITY

        wifi.setTransport(&transport);
        char buffer[256];
        sensor.toJson(buffer, sizeof(buffer));
        int16_t response = wifi.send("sensors", buffer);
        if (response != 200 && response != 201)
        {
            Serial.printf("POST Failed: %d\n", response);
            WebSerial.printf("POST Failed: %d\n", response);
        }
    }
}

void onDataHandler(ModbusMessage response, uint32_t token)
{
    errorTransactionModbusCounter = 0; // Reset error counter since transaction work again

    uint16_t offset = 3;
    offset = response.get(offset, sensorDatas[0]);
    offset = response.get(offset, sensorDatas[1]);
    offset = response.get(offset, sensorDatas[2]);
    offset = response.get(offset, sensorDatas[3]);
    offset = response.get(offset, sensorDatas[4]);
    Serial.printf("[INFO] Success Parse: [%u, %u, %u, %u, %u]\n",
                  sensorDatas[0], sensorDatas[1], sensorDatas[2], sensorDatas[3], sensorDatas[4]);
}

void onErrorHandler(Error error, uint32_t token)
{
    Serial.printf("[ERROR] token: %d | code: %d\n", token, error);
    WebSerial.printf("[ERROR] token: %d | code: %d\n", token, error);

    if (errorTransactionModbusCounter > 10)
        ESP.restart();
    errorTransactionModbusCounter++;
}

void onDataHandlerArr(ModbusMessage response, uint32_t token)
{
    errorTransactionModbusArrCounter = 0; // Reset error counter since transaction work again

    uint16_t offset = 3;
    offset = response.get(offset, rainfallGravityData);
    Serial.printf("[INFO] Success Parse: [%u]\n",
                  rainfallGravityData);
}

void onErrorHandlerArr(Error error, uint32_t token)
{
    Serial.printf("[ERROR] token: %d | code: %d\n", token, error);
    WebSerial.printf("[ERROR] token: %d | code: %d\n", token, error);

    if (errorTransactionModbusArrCounter > 10)
        ESP.restart();
    errorTransactionModbusArrCounter++;
}

void fetchOpenMeteoData()
{
    HTTPClient http;
    const char *endpoint = "https://api.open-meteo.com/v1/forecast?latitude=-7.0500&longitude=107.5500&current=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m,wind_direction_10m&wind_speed_unit=ms&timezone=Asia%2FJakarta";

    http.begin(endpoint);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200)
    {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.printf("[ERROR] JSON Deserialization failed: %s\n", error.c_str());
            WebSerial.printf("[ERROR] JSON Deserialization failed: %s\n", error.c_str());
        }
        else
        {
            float temp = doc["current"]["temperature_2m"];
            float hum = doc["current"]["relative_humidity_2m"];
            float precip = doc["current"]["precipitation"];
            float ws = doc["current"]["wind_speed_10m"];
            int wd_deg = doc["current"]["wind_direction_10m"];

            sensorDatas[0] = static_cast<uint16_t>(temp * 10.0f);
            sensorDatas[1] = static_cast<uint16_t>(hum * 10.0f);
            // sensorDatas[2] = static_cast<uint16_t>(precip * 10.0f);
            sensorDatas[3] = static_cast<uint16_t>(ws * 10.0f);
            sensorDatas[4] = static_cast<uint16_t>(((wd_deg + 22) / 45) % 8);

            Serial.println("[INFO] Successfully fetched and mapped Open-Meteo data");
            WebSerial.println("[INFO] Successfully fetched and mapped Open-Meteo data");
        }
    }
    else
    {
        Serial.printf("[ERROR] Open-Meteo HTTP Request failed, code: %d\n", httpResponseCode);
        WebSerial.printf("[ERROR] Open-Meteo HTTP Request failed, code: %d\n", httpResponseCode);
    }

    http.end();
}