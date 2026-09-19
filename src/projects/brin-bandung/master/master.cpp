#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>

#define DEVICE_ID 0

#include "../config.h" // .env
#include "../utils/ntp_service.h"
#include "services/application.h"
#include "services/command_parser.h"
#include "transmitter/configs/wifi_module.h"

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";

void setup()
{
    Serial.begin(115200);
    Serial.printf("Last Reset Reason: %s\n", Parser::parseResetReasonESP(esp_reset_reason()));

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "T4T-WS-Master-%d", DEVICE_ID + 1);
    static WiFiModule wifi(ssid, password, hostname, WIFI_POWER_8_5dBm);

    static sharedModbusClientContext sharedMBClient(Serial1);
    static contextMBWS wsCtx(Serial1, sharedMBClient);
    static contextMBRainfall rainCtx(Serial1, sharedMBClient);
    static contextDisplay displayCtx(SPI);
    static contextPublisher publisherCtx(wifi);
    static contextDaemon daemonCtx(wifi);

    Application::init();

    xTaskCreate(Application::taskReadRainfall, "sampling WD task", 4096, &rainCtx, 3, &handleReadRainfall);
    xTaskCreate(Application::taskReadWS, "modbus read rainfall slave task", 4096, &wsCtx, 2, &handleMBSlave);
    xTaskCreate(Application::taskDisplayDashboard, "display dashboard task", 4096, &displayCtx, 2, &handleDisplay);
    xTaskCreate(Application::taskSendSupabase, "send supabase task", 8192, &publisherCtx, 1, &handlePublish);
    xTaskCreate(Application::taskPollOta, "ota polling task", 4096, &daemonCtx, 1, &handleOta);
    xTaskCreate(Application::taskDaemon, "daemon task", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }