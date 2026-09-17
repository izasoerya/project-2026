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
#include "../utils/ntp_service.h"
#include "transmitter/configs/wifi_module.h"
#include "services/application.h"

/**
 * @brief DEVICE SELECTION
 *
 * [0] = CISANGKUY
 * [1] = CIMINYAK
 */
#define DEVICE_ID 0

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";

void setup()
{
    Serial.begin(115200);

    char hostname[64];
    snprintf(hostname, sizeof(hostname), "slave-arr-bandung-persemaian-%d.local", DEVICE_ID + 1);
    WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);
    static contextDaemon daemonCtx(wifi);
    daemonCtx.setInternetStatus(FeatureStatus::INTERNET); // Default to not working
    daemonCtx.setNTPStatus(FeatureStatus::NTP);           // Default to not working

    if (daemonCtx.getInternetStatus() == FeatureStatus::WORKING)
    {
        if (NTPService::init())
            daemonCtx.setNTPStatus(FeatureStatus::WORKING);

        AsyncWebServer server(80);
        Application app;
        ElegantOTA.setAutoReboot(true);
        ElegantOTA.begin(&server);
        ElegantOTA.onEnd([](bool success)
                         { if(success) esp_restart(); });
        WebSerial.begin(&server);
        server.begin();

        WireGuard wg;
        WireGuardConfig wgConfig = wgConfigs[DEVICE_ID];
        IPAddress wgLocalIP;
        wgLocalIP.fromString(wgConfig.slave.localIp);
        Serial.printf("wg ip: %s\n", wgLocalIP.toString());
        bool wgOk = wg.begin(wgLocalIP, wgConfig.slave.privateKey,
                             WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
        if (!wgOk)
        {
            // TODO: HANDLE IF WIREGUARD FAIL
        }
    }

    Application::init();

    static contextTHWS sensorCtx(Wire);
    static contextMB mbCtx(Serial1);
    static contextWD wdCtx(Serial0);
    xTaskCreate(Application::taskReadTHWS, "sampling THWS task", 8192, &sensorCtx, 2, &handleReadTHWS);
    xTaskCreate(Application::taskReadWD, "sampling WD task", 8192, &wdCtx, 3, &handleReadWD);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 8192, &mbCtx, 1, &handleMBSlave);
    xTaskCreate(Application::taskDaemon, "daemon task", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }