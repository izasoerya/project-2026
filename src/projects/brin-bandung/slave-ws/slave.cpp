#include <Arduino.h>
#include <esp_task_wdt.h>
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
    static WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);
    static contextDaemon daemonCtx(wifi);
    daemonCtx.setInternetStatus(FeatureStatus::INTERNET); // Default to not working
    daemonCtx.setNTPStatus(FeatureStatus::NTP);           // Default to not working

    Application::init();

    static contextTHWS sensorCtx(Wire);
    static contextMB mbCtx(Serial1);
    static contextWD wdCtx(Serial0);
    xTaskCreate(Application::taskReadTHWS, "sampling THWS task", 8192, &sensorCtx, 3, &handleReadTHWS);
    xTaskCreate(Application::taskReadWD, "sampling WD task", 8192, &wdCtx, 4, &handleReadWD);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 8192, &mbCtx, 1, &handleMBSlave);
    xTaskCreate(Application::taskPollOta, "ota task", 8192, nullptr, 2, &handleOta);
    xTaskCreate(Application::taskDaemon, "daemon task", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }