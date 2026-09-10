#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <WireGuard-ESP32.h>

#include "../config.h" // .env
#include "../utils/parser.h"
#include "../utils/utils.h"
#include "../services/ntp_service.h"
#include "transmitter/configs/wifi_module.h"
#include "services/application.h"

/**
 * @brief DEVICE SELECTION
 *
 * [0] = CISANGKUY
 * [1] = CIMINYAK
 */
#define DEVICE_ID 1

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "slave-bandung-persemaian-2";
WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);

WireGuard wg;
AsyncWebServer server(80);
WireGuardConfig wgConfig = wgConfigs[DEVICE_ID];
Application app;

void setup()
{
    Serial.begin(115200);
    if (wifi.begin([]()
                   { Serial.println("."); }, []()
                   { esp_restart(); }))
        Serial.println(wifi.localIP());

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server);
    ElegantOTA.onEnd([](bool success)
                     { if(success) esp_restart(); });
    WebSerial.begin(&server);
    server.begin();

    NTPService::init();
    IPAddress wgLocalIP;
    wgLocalIP.fromString(wgConfig.slave.localIp);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, wgConfig.slave.privateKey,
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

    static contextTHWS sensorCtx(Wire);
    static contextMB mbCtx(Serial1);
    static contextWD wdCtx(Serial2);

    xTaskCreate(Application::taskReadTHWS, "sampling THWS task", 8192, &sensorCtx, 2, &handleReadTHWS);
    xTaskCreate(Application::taskReadWD, "sampling WD task", 8192, &wdCtx, 3, &handleReadWD);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 8192, &mbCtx, 1, &handleMBSlave);
}

void loop() { vTaskDelay(portMAX_DELAY); }