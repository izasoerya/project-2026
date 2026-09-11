#include <TFT_eSPI.h>
#include <ModbusClientTCPasync.h>
#include <time.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <WireGuard-ESP32.h>

#include "config.h" // .env
#include "models.h"
#include "utils/ntp_service.h"
#include "display/display_tft_spi_lcd/display_tft.h"
#include "transmitter/configs/wifi_module.h"
#include <ModbusClientRTU.h>

/**
 * @brief DEVICE SELECTION
 *
 * [0] = CISANGKUY
 * [1] = CIMINYAK
 */
#define DEVICE_ID 0

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
WireGuardConfig wgConfig = wgConfigs[DEVICE_ID];

ModbusClientRTU modbusClient(3);
ModbusClientRTU modbusArrClient(3);
void onDataHandler(ModbusMessage response, uint32_t token);
void onErrorHandler(Error error, uint32_t token);
void onDataHandlerArr(ModbusMessage response, uint32_t token);
void onErrorHandlerArr(Error error, uint32_t token);

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

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 6, 7);

    if (wifi.begin([]()
                   { Serial.print("."); }, []()
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

    if (NTPService::init())
    {
        Serial.println("NTP Initialization Error");
        WebSerial.println("NTP Initialization Error");
    }

    IPAddress wgLocalIP;
    wgLocalIP.fromString(wgConfig.slaveLocalIp);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());

    bool wgOk = wg.begin(wgLocalIP, wgConfig.masterPrivateKey,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
    if (wgOk)
        Serial.println("WireGuard successfully initialized on ESP32!");
    else
        Serial.println("WireGuard initialization failed!");

    modbusClient.onDataHandler(&onDataHandler);
    modbusClient.onErrorHandler(&onErrorHandler);
    modbusClient.setTimeout(10000);
    modbusClient.begin(Serial1);

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

        TimeStruct ts = NTPService::getTime();
        if (ts.isValid())
        {
            display.setClock(ts.hour, ts.minute, ts.second);
            display.setSignalStrength(wifi.getRssi());
            display.refresh();
        }
    }

    if (millis() - prevSystemLoggingMillis > 60000 * 60) // Every 1 hour
    {
        prevSystemLoggingMillis = millis();

        SystemLogDto systemLog{
            .deviceId = DEVICE_ID + 1,
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

        Error errArr = modbusArrClient.addRequest(
            (uint32_t)stampDataModbusArrCounter, // Token
            1, READ_HOLD_REGISTER, 0, 1);
        if (errArr != SUCCESS)
        {
            ModbusError e(errArr);
            Serial.printf("Error ARR creating request: %02X - %s\n", (int)e, (const char *)e);
            WebSerial.printf("Error ARR creating request: %02X - %s\n", (int)e, (const char *)e);
        }
        stampDataModbusArrCounter++;

        SensorDto sensor{
            .deviceId = DEVICE_ID + 1,
            .rainFall = float(rainfallGravityData / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };
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

    if (millis() - prevTransmitMillis > 60000 * 5) // Every 5 minute
    {
        prevTransmitMillis = millis();

        SensorDto sensor{
            .deviceId = DEVICE_ID + 1,
            .rainFall = float(rainfallGravityData / 10.0F),
            .windSpeed = float(sensorDatas[3] / 10.0F),
            .windDirection = static_cast<WindDirectionEnum>(sensorDatas[4]),
            .airTemperature = float(sensorDatas[0] / 10.0F),
            .airHumidity = float(sensorDatas[1] / 10.0F),
            .rainFallGravity = float(rainfallGravityData / 10.0F),
        };

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
        esp_restart();
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