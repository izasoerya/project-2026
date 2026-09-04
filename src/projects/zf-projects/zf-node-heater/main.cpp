#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include "services/request_job.h"
#include "models/sensor.h"
#include "models/actuator_mode.h"

#define PIN_RELAY D6

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "zf-node-heater-1";

void automationTask(Actuator actuator);

AsyncWebServer server(80);
RequestJob req(automationTask);
Ticker scheduler;

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, HIGH);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
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
    scheduler.attach(20, [&]()
                     { req.requestActuatorData(); });
}

void automationTask(Actuator actuator)
{
    bool prevData = !actuator.state; // Active Low
    digitalWrite(PIN_RELAY, prevData);
    bool actualValue = digitalRead(PIN_RELAY);
    if (prevData != actualValue)
    {
        // TODO: HANDLE BUG OR NOTIF THE USER
        Serial.println("Discrepancy output detected!");
        WebSerial.println("Discrepancy output detected!");
    }

    Serial.printf("RELAY: %s\n", actualValue ? "OFF" : "ON");
    WebSerial.printf("RELAY: %s\n", actualValue ? "OFF" : "ON");
}

void loop()
{
    MDNS.update();
    ElegantOTA.loop();
    WebSerial.loop();
}