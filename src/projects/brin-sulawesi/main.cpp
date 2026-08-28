#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include "consts/sensors.h"
#include "services/application.h"
#include "services/appstate_parser.h"
#include "transmitter/configs/wifi_module.h"
#include "transmitter/configs/mqtt_module.h"
#include "models/shared_modbus_obj.h"

#define DEVICE_ID 1

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";
const char *hostname = "wtq-1";
WiFiModule inet(ssid, password, hostname, WIFI_POWER_19_5dBm);
MQTTModule mqtt("username", "password", "lala.land");

Modbustatics *turbSensor;
Modbustatics *awlrSensor;
QueueHandle_t Application::_turbidityQueue = NULL;
QueueHandle_t Application::_awlrQueue = NULL;

AsyncWebServer server(80);
CommandHandler handler;

TaskHandle_t mainTaskHandle;
TaskHandle_t samplingTaskHandle;
TaskHandle_t calibrateHandle;

void setup()
{
    Preferences prefs;
    prefs.begin("app_config", true);
    const uint8_t firmwareId = prefs.getUChar("id", 0);
    prefs.end();
    if (firmwareId == 0)
    {
        prefs.begin("app_config", false);
        prefs.putUChar("id", DEVICE_ID);
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
    static ApplicationContext ctx; // Mutex lock context variable
    ctx.state = AppState::NORMAL;
    ctx.mbTurbidity = turbSensor;
    ctx.mbAwlr = awlrSensor;
    prefs.begin("app_config", true);
    bool mqttRetainOn = prefs.getBool("mqtt_retain_on", 255); // return 255 means no index yet
    ctx.feature.isMQTTEnabled = mqttRetainOn == 255 ? true : mqttRetainOn;
    prefs.end();
    Application::initTask();

    // clang-format off
    inet.begin( []() { Serial.print("."); },    // On progress
                []() { esp_restart; });         // On timeout
    // clang-format on
    Serial.printf("Connected with: %s\n", inet.localIP());
    WebSerial.printf("Connected with: %s\n", inet.localIP());
    if (ctx.feature.isMQTTEnabled)
    {
        if (!mqtt.connect())
        {
            Serial.println("Failed to connect broker MQTT");
            WebSerial.println("Failed to connect broker MQTT");
        }
    }
    else
    {
        Serial.println("MQTT is disabled");
        WebSerial.println("MQTT is disabled");
    }

    server.begin();
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    WebSerial.begin(&server, "/webserial");
    WebSerial.onMessage(
        [&](uint8_t *data, size_t len)
        {
            ctx.state = handler.parseCommand(data, len);
            if (ctx.state == AppState::SET_FEATURE)
            {
                FeaturesEnum feature = handler.parseFeatureCommand(data, len);
                prefs.begin("app_config", false);
                prefs.putBool("mqtt_retain_on",
                              feature == FeaturesEnum::MQTT_RETAIN_ON ? true : false);
                prefs.end();
            }
        });

    xTaskCreate(Application::mainTask, "main task", 8192, &ctx, 1, &mainTaskHandle);
    xTaskCreate(Application::samplingTask, "sampling task", 8192, &ctx, 1, &samplingTaskHandle);
    xTaskCreate(Application::calibrateTask, "calibrate turbidity", 4096, &ctx, 1, &calibrateHandle);
}

void loop() { vTaskDelay(portMAX_DELAY); }