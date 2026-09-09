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
#include "models/shared_modbus_obj.h"

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
    Serial2.begin(9600);
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

    static MQTTContext mqttCtx;
    mqttCtx.isInetnetEnabled = WiFi.isConnected();
    mqttCtx.isEnabled = true;

    Application::init();

    // clang-format off
    inet.begin( []() { Serial.print("."); },    // On progress
                []() { esp_restart; });         // On timeout
    // clang-format on
    Serial.printf("Connected with: %s\n", inet.localIP());
    WebSerial.printf("Connected with: %s\n", inet.localIP());

    server.begin();
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");
    WebSerial.onMessage(
        [&](uint8_t *data, size_t len)
        {
            handler.CommonCommand(data, len);
            ctx.state = handler.parseCommand(data, len);
            if (ctx.state == AppState::SET_FEATURE)
            {
                FeaturesEnum feature = handler.parseFeatureCommand(data, len);
                if (feature == FeaturesEnum::MQTT_RETAIN_ON || feature == FeaturesEnum::MQTT_RETAIN_OFF)
                {
                    prefs.begin("app_config", false);
                    prefs.putBool("mqtt_retain_on",
                                  feature == FeaturesEnum::MQTT_RETAIN_ON ? true : false);
                    prefs.end();
                }
                else if (feature == FeaturesEnum::MQTT_ON || feature == FeaturesEnum::MQTT_OFF)
                {
                    mqttCtx.isEnabled = feature == FeaturesEnum::MQTT_ON ? true : false;
                }
            }
        });

    xTaskCreate(Application::publisherTask, "publisher task", 8192, &ctx, 2, &mainTaskHandle);
    xTaskCreate(Application::samplingTask, "sampling task", 8192, &ctx, 3, &samplingTaskHandle);
    xTaskCreate(Application::calibrateTask, "calibrate turbidity", 4096, &ctx, 3, &calibrateHandle);
    xTaskCreate(Application::notifierTask, "notifier task", 4096, &ctx, 1, &notifierTaskHandle);
    xTaskCreate(Application::mqttThreadTask, "mqtt thread task", 4096, &mqttCtx, 1, &mqttThreadTaskHandle);
}

void loop() { vTaskDelay(portMAX_DELAY); }