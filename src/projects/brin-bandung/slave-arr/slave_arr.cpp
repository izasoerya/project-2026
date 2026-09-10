#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>
#include <DFRobot_RainfallSensor.h>

#include "../config.h" // .env
#include "../services/ntp_service.h"
#include "services/application.h"
#include "transmitter/configs/wifi_module.h"

#define DEVICE_ID 0

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "slave-arr-bandung-persemaian-1"; //! RECHECK THIS EVERYTIME COMPILE
WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);

AsyncWebServer server(80);
WireGuard wg;
WireGuardConfig wgConfig = wgConfigs[DEVICE_ID];

/**
 * @brief Pinout note
 * - Slave ARR-1 (SDA = 5, SCL = 6)
 * - Slave ARR-2 (SDA = 7, SCL = 6)
 */
const uint8_t pinSDA = 5; // TODO: CHANGE TO APPROPRIATE PIN
const uint8_t pinSCL = 6; // TODO: CHANGE TO APPROPRIATE PIN

void setup()
{
    Serial.begin(115200);

    if (wifi.begin([]() -> void
                   { Serial.print("."); }, []() -> void
                   { esp_restart(); }))
        Serial.printf("Connected with IP: %s", wifi.localIP());

    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onEnd([](bool success)
                     {if (success) esp_restart(); });
    ElegantOTA.begin(&server);
    WebSerial.begin(&server);
    server.begin();

    if (NTPService::init())
    {
        // TODO: HANDLE IF NTP FAIL
    }
    IPAddress wgLocalIP;
    wgLocalIP.fromString(wgConfig.slaveArr.localIp);
    Serial.printf("wg ip: %s\n", wgLocalIP.toString());
    bool wgOk = wg.begin(wgLocalIP, wgConfig.slaveArr.privateKey,
                         WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
    if (!wgOk)
    {
        // TODO: HANDLE IF WG FAIL
    }

    static contextRainfall ctx(Wire);
    xTaskCreate(Application::taskReadRainfall, "sampling WD task", 8192, &ctx, 3, &handleReadRainfall);
}

void loop() { vTaskDelay(portMAX_DELAY); }
