#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <WireGuard-ESP32.h>
#include <esp_task_wdt.h>
#include <LiquidCrystal_I2C.h>

#include "config.h" // .env
#include "transmitter/configs/wifi_module.h"

const char *ssid = "Subhanallah";
const char *password = "muhammadnabiyullah";
const char *hostname = "bandung-spas-1";
WiFiModule wifi(ssid, password, hostname);
AsyncWebServer server(80);
WireGuard wg;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4);
bool isLCDExist = false;

bool isI2CDevicePresent(uint8_t address)
{
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0); // 0 = ACK received, device exists
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    esp_task_wdt_init(60, true);
    esp_task_wdt_add(NULL);

    if (wifi.begin())
        Serial.printf("Connected with IP: %s\n", wifi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Test Successful!"); });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
              { ESP.restart(); });

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server);
    WebSerial.begin(&server);
    server.begin();

    if (isI2CDevicePresent(0x27))
    {
        lcd.init(); // initialize the lcd
        lcd.backlight();
        isLCDExist = true;
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
            Serial.println("NTP Sync Failed! Restarting...");
            WebSerial.println("NTP Sync Failed! Restarting...");
            ESP.restart(); // Critical: WireGuard handshake will fail without correct time
        }
        retryCounter++;
    }

    IPAddress wgLocalIP;
    wgLocalIP.fromString(WG_DEVICE_LOCAL_IP);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, WG_DEVICE_PRIVATE_KEY,
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
}

uint32_t prevLogTime = 0;

void loop()
{
    esp_task_wdt_reset();
    wifi.reconnect();
    ElegantOTA.loop();

    if (millis() - prevLogTime >= 10000)
    {
        prevLogTime = millis();

        Serial.printf("RSSI: %d dBm\n", wifi.getRssi());
        WebSerial.printf("RSSI: %d dBm\n", wifi.getRssi());

        if (isLCDExist)
        {
            lcd.setCursor(0, 0);
            lcd.print("=== BRIN  WATERQ-1 ===");
            lcd.setCursor(0, 1);
            lcd.printf("RSSI: %d dBm", wifi.getRssi());
        }
    }
}