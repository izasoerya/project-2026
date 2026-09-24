#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include "consts/global_config.h"
#include "consts/sensors.h"
#include "models/profile.h"
#include "services/application.h"
#include "services/appstate_parser.h"
#include "transmitter/configs/wifi_module.h"
#include "transmitter/configs/mqtt_module.h"
#include "models/task_context.h"

WiFiModule inet(
    GlobalConfig::ssid,
    GlobalConfig::password,
    GlobalConfig::hostname,
    WIFI_POWER_19_5dBm);

Modbustatics *turbSensor = nullptr;
Modbustatics *awlrSensor = nullptr;

AsyncWebServer server(80);
CommandHandler handler;

void setup()
{
    Preferences prefs;
    prefs.begin("app_config", true);
    const uint8_t firmwareId = prefs.getUChar("id", 0);
    prefs.end();
    if (firmwareId == 0)
    {
        prefs.begin("app_config", false);
        prefs.putUChar("id", GlobalConfig::deviceID);
        prefs.end();
        esp_restart();
    }
    Serial.begin(115200);
    Serial.printf("Firmware ID: %u\n", firmwareId);

    for (int i = 0; i < sizeof(turb_list) / sizeof(Modbustatics); i++)
    {
        if (turb_list[i].getId() == firmwareId)
            turbSensor = &turb_list[i];
        if (awlr_list[i].getId() == firmwareId)
            awlrSensor = &awlr_list[i];
    }

    const float batteryDangerLevel = 11.75;
    const float batteryWarningLevel = 12.0;
    Profile batteryProfileDefault(batteryDangerLevel, batteryWarningLevel);

    static ApplicationContext ctx; // Mutex lock context variable
    ctx.state = AppState::NORMAL;
    ctx.mbTurbidity = turbSensor;
    ctx.mbAwlr = awlrSensor;
    ctx.batteryProfile = &batteryProfileDefault;

    static NetworkingContext networkCtx(inet);

    xTaskCreate(Application::sensorAgregatorTask, "publisher task", 8192, &ctx, 2, &mainTaskHandle);
    xTaskCreate(Application::samplingTask, "sampling task", 8192, &ctx, 3, &samplingTaskHandle);
    xTaskCreate(Application::calibrateTask, "calibrate turbidity", 4096, &ctx, 3, &calibrateHandle);
    xTaskCreate(Application::notifierTask, "notifier task", 4096, &ctx, 1, &notifierTaskHandle);
}

void loop() { vTaskDelay(portMAX_DELAY); }