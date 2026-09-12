#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>

#define DEVICE_ID 0

#include "../config.h" // .env
#include "../utils/ntp_service.h"
#include "services/application.h"
#include "transmitter/configs/wifi_module.h"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";

void setup()
{
    Serial.begin(115200);

    char hostname[64];
    snprintf(hostname, sizeof(hostname), "master-bandung-persemaian-%d.local", DEVICE_ID + 1);
    WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);
    if (wifi.begin([]() -> void
                   { Serial.print("."); }, []() -> void
                   { esp_restart(); }))
        Serial.printf("Connected with IP: %s", wifi.localIP());

    static contextMBWS wsCtx(Serial0);
    static contextMBRainfall rainCtx(Serial1);
    static contextDisplay displayCtx(SPI);
    static contextDaemon daemonCtx(wifi);
    static contextPublisher publisherCtx(wifi);

    WireGuard wg;
    WireGuardConfig wgConfig = wgConfigs[DEVICE_ID];
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onEnd([](bool success)
                     {if (success) esp_restart(); });

    AsyncWebServer server(80);
    ElegantOTA.begin(&server);
    WebSerial.begin(&server);
    server.begin();

    if (NTPService::init())
        daemonCtx.setNTPStatus(FeatureStatus::NTP);

    IPAddress wgLocalIP;
    wgLocalIP.fromString(wgConfig.master.localIp);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, wgConfig.master.privateKey,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
    if (!wgOk)
        daemonCtx.setWireGuardStatus(FeatureStatus::WIREGUARD);

    xTaskCreate(Application::taskReadRainfall, "sampling WD task", 4096, &rainCtx, 3, &handleReadRainfall);
    xTaskCreate(Application::taskReadWS, "modbus read rainfall slave task", 4096, &wsCtx, 2, &handleMBSlave);
    xTaskCreate(Application::taskDaemon, "daemon task", 4096, &daemonCtx, 1, &handleDaemon);
    xTaskCreate(Application::taskDisplayDashboard, "display dashboard task", 4096, &displayCtx, 2, &handleDisplay);
    xTaskCreate(Application::taskSendSupabase, "send supabase task", 8192, &publisherCtx, 1, &handlePublish);
}

void loop() { vTaskDelay(portMAX_DELAY); }
