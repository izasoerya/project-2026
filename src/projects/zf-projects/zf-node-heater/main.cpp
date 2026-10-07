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

#define FLOOR_ID 2
#define PIN_RELAY 5
#define PIN_G_LED 12
#define BOT_TOKEN "8738540069:AAG1bONoND4JkHNQ_zHLFNh7c6MGEOITWoU"
#define CHAT_ID "6720768632"

const char *ssid = "NodeSensorWiFi";
const char *password = "muhammadnabiyullah";
void automationTask(Actuator actuator);

AsyncWebServer server(80);
RequestJob req(FLOOR_ID, automationTask);

WiFiClientSecure wClient;
UniversalTelegramBot bot(BOT_TOKEN, wClient);
X509List cert(TELEGRAM_CERTIFICATE_ROOT);

#define PIN_BUTTON D2 // GPIO4 - adjust if needed for your ESP Witty board

volatile uint32_t lastButtonTime = 0;
volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR()
{
    uint32_t now = millis();
    if (now - lastButtonTime > 50)
    {
        if (digitalRead(PIN_BUTTON) == LOW)
        {
            buttonPressed = true;
            lastButtonTime = now;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_G_LED, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(PIN_G_LED, LOW);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), buttonISR, CHANGE);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    wClient.setTrustAnchors(&cert);

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "zf-node-heater-%d", FLOOR_ID);
    WiFi.hostname(hostname);
    WiFi.begin(ssid, password);
    uint32_t timestampConnect = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(WiFi.status());
        delay(500);

        if (millis() - timestampConnect < 30000) // Timeout
            ESP.restart();
    }

    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected with IP: %d.%d.%d.%d\n", localIP[0], localIP[1], localIP[2], localIP[3]);
    MDNS.begin(hostname);
    MDNS.addService("http", "tcp", 80);

    server.begin();
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");

    req.begin();
}

void automationTask(Actuator actuator)
{
    bool prevData = actuator.state; // Active Low
    digitalWrite(PIN_RELAY, prevData);
    bool actualValue = digitalRead(PIN_RELAY);
    if (prevData != actualValue)
    {
        Serial.println("Discrepancy output detected!");
        WebSerial.println("Discrepancy output detected!");
        bot.sendMessage(CHAT_ID, "Discrepancy output detected!", "");
    }

    Serial.printf("RELAY: %s\n", actualValue ? "ON" : "OFF");
    WebSerial.printf("RELAY: %s\n", actualValue ? "ON" : "OFF");
}

void loop()
{
    MDNS.update();
    ElegantOTA.loop();
    WebSerial.loop();

    if (buttonPressed)
    {
        buttonPressed = false;
        digitalWrite(PIN_RELAY, !digitalRead(PIN_RELAY));
        digitalWrite(PIN_G_LED, !digitalRead(PIN_G_LED));

        Serial.printf("Manual button relay: %d\n", digitalRead(PIN_RELAY));
        WebSerial.printf("Manual button relay: %d\n", digitalRead(PIN_RELAY));
    }

    static uint32_t prevLog = 0;
    if (millis() - prevLog > 1000)
    {
        prevLog = millis();

        Serial.printf("RSSI: %d\n", WiFi.RSSI());
        WebSerial.printf("RSSI: %d\n", WiFi.RSSI());
    }

    static uint32_t prevAutomation = 0;
    if (millis() - prevAutomation > 20000)
    {
        prevAutomation = millis();
        req.requestActuatorData();
    }
}