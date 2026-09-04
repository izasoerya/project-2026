#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include "services/request_job.h"
#include "models/sensor.h"
#include "models/actuator_mode.h"
#include <UniversalTelegramBot.h>

#define FLOOR_ID 1
#define PIN_RELAY D6
#define BOT_TOKEN "8738540069:AAG1bONoND4JkHNQ_zHLFNh7c6MGEOITWoU"
#define CHAT_ID "6720768632"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
void automationTask(Actuator actuator);

AsyncWebServer server(80);
RequestJob req(FLOOR_ID, automationTask);

WiFiClientSecure wClient;
UniversalTelegramBot bot(BOT_TOKEN, wClient);
X509List cert(TELEGRAM_CERTIFICATE_ROOT);

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, HIGH);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    wClient.setTrustAnchors(&cert);

    char hostname[64];
    snprintf(hostname, sizeof(hostname), "zf-node-heater-%d", FLOOR_ID);
    WiFi.hostname(hostname);
    unsigned char n = WiFi.scanNetworks();
    for (unsigned char i = 0; i < n; i++)
        Serial.printf("%d: %s (RSSI: %d)\n", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(WiFi.status());
        delay(500);
    }
    configTime(3600 * 7, 0, "pool.ntp.org");
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 24 * 3600 && attempts < 100) // While time < Jan 2, 1970
    {
        delay(100);
        now = time(nullptr);
        attempts++;
    }
    Serial.printf("Time synced: %s\n", ctime(&now));

    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected with IP: %d.%d.%d.%d\n", localIP[0], localIP[1], localIP[2], localIP[3]);
    MDNS.begin(hostname);
    MDNS.addService("http", "tcp", 80);

    server.begin();
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");
    WebSerial.onMessage(
        [&](uint8_t *data, size_t len) {

        });

    req.begin();
}

void automationTask(Actuator actuator)
{
    bool prevData = !actuator.state; // Active Low
    digitalWrite(PIN_RELAY, prevData);
    bool actualValue = digitalRead(PIN_RELAY);
    if (prevData != actualValue)
    {
        Serial.println("Discrepancy output detected!");
        WebSerial.println("Discrepancy output detected!");
        bot.sendMessage(CHAT_ID, "Discrepancy output detected!", "");
    }

    Serial.printf("RELAY: %s\n", actualValue ? "OFF" : "ON");
    WebSerial.printf("RELAY: %s\n", actualValue ? "OFF" : "ON");
}

void loop()
{
    MDNS.update();
    ElegantOTA.loop();
    WebSerial.loop();

    static uint64_t prevAutomation = 0;
    if (millis() - prevAutomation > 20000)
    {
        prevAutomation = millis();
        req.requestActuatorData();
    }
}