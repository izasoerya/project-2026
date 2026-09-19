#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>
#include <DFRobot_RainfallSensor.h>

#include "../config.h" // .env
#include "../utils/ntp_service.h"
#include "services/application.h"
#include "transmitter/configs/wifi_module.h"

#define DEVICE_ID 0

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";

void setup()
{
    Serial.begin(115200);

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "TFT-ARR-SLAVE-%d", DEVICE_ID + 1);
    static WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);
    static contextDaemon daemonCtx(wifi);

    Application::init();
    configTime(7 * 3600, 0, nullptr, nullptr, nullptr);

    static contextRainfall rainCtx(Wire);
    static contextMB mbCtx(Serial0);
    xTaskCreate(Application::taskReadRainfall, "sampling WD task", 4092, &rainCtx, 3, &handleReadRainfall);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 4092, &mbCtx, 2, &handleMBSlave);
    xTaskCreate(Application::taskPollOta, "ota task", 4096, &daemonCtx, 2, &handleOta);
    xTaskCreate(Application::taskDaemon, "daemon", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }
