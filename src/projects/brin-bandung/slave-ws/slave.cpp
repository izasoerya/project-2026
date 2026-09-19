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

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "T4T-WS-SLAVE-%d", DEVICE_ID + 1);
    static WiFiModule wifi(ssid, password, hostname, WIFI_POWER_8_5dBm);
    static contextDaemon daemonCtx(wifi);
    daemonCtx.setInternetStatus(FeatureStatus::INTERNET); // Default to not working
    daemonCtx.setNTPStatus(FeatureStatus::NTP);           // Default to not working

    Application::init();
    configTime(7 * 3600, 0, nullptr, nullptr, nullptr);

    static contextTHWS sensorCtx(Wire);
    static contextMB mbCtx(Serial1);
    static contextWD wdCtx(Serial0);
    xTaskCreate(Application::taskReadTHWS, "sampling THWS task", 4096, &sensorCtx, 3, &handleReadTHWS);
    xTaskCreate(Application::taskReadWD, "sampling WD task", 3072, &wdCtx, 4, &handleReadWD);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 4096, &mbCtx, 1, &handleMBSlave);
    xTaskCreate(Application::taskPollOta, "ota task", 4096, &daemonCtx, 2, &handleOta);
    xTaskCreate(Application::taskDaemon, "daemon task", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }