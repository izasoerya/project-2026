#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include "transmitter/configs/mqtt_module.h"

#include "services/application.h"
#include "services/request_job.h"
#include "models/sensor.h"
#include "models/actuator_mode.h"

RequestJob *req = nullptr;
MQTTModule mqttClient(GlobalConfig::usernameMQTT, GlobalConfig::passwordMQTT, GlobalConfig::brokerMQTT, 8883);

void setup()
{
    Serial.begin(115200);

    static ActuatorDriver sharedActuatorDriver;
    static ActuatorDriverContext driverCtx(sharedActuatorDriver);
    attachInterruptArg(digitalPinToInterrupt(GlobalConfig::PIN_BUTTON),
                       Application::buttonISR,
                       &driverCtx,
                       CHANGE);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "zf-node-heater-%d", GlobalConfig::FLOOR_ID);
    WiFi.hostname(hostname);
    WiFi.begin(GlobalConfig::ssid, GlobalConfig::password);
    uint32_t timestampConnect = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(WiFi.status());
        delay(500);

        if (millis() - timestampConnect > 30000) // Timeout
            ESP.restart();
    }

    IPAddress localIP = WiFi.localIP();
    Serial.printf("Connected with IP: %d.%d.%d.%d\n", localIP[0], localIP[1], localIP[2], localIP[3]);
    MDNS.begin(hostname);
    MDNS.addService("http", "tcp", 80);

    static AsyncWebServer server(80);
    server.begin();
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");

    if (!mqttClient.connect())
    {
        Serial.println("Failed to connect MQTT, there will be no log timeout");
        WebSerial.println("Failed to connect MQTT, there will be no log timeout");
    }

    static AutomationActuatorContext automationActuatorCtx(sharedActuatorDriver, mqttClient);
    req = new RequestJob(GlobalConfig::FLOOR_ID, [](Actuator act)
                         { Application::automationActuatorCb(act, &driverCtx); }, []()
                         { Application::timeoutActautorCb(&automationActuatorCtx); });
    req->begin();
}

void loop()
{
    MDNS.update();
    ElegantOTA.loop();
    WebSerial.loop();
    mqttClient.reconnect();

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
        req->requestActuatorData();
    }
}