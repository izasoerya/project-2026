#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ElegantOTA.h>
#include <WebSerial.h>

#include "../consts/sensors.h"
#include "../models/task_context.h"
#include "../models/datastore.h"

SensorDataStore globalSensorStore;

static TaskHandle_t mainTaskHandle;
static TaskHandle_t samplingTaskHandle;
static TaskHandle_t calibrateHandle;
static TaskHandle_t notifierTaskHandle;
static TaskHandle_t mqttThreadTaskHandle;

static QueueHandle_t queueSensorAgregator;
static QueueHandle_t queueSensorPublish;

struct MqttPayload
{
    const char *topic;
    const char *message;
};

class Application
{
public:
    static void init()
    {
        queueSensorPublish = xQueueCreate(10, sizeof(MqttPayload));
        queueSensorAgregator = xQueueCreate(10, sizeof(SensorObject));
    }

    static void networkingTask(void *pvParam)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);
        bool isWiFiConnected = false;
        ctx->wifi.beginNB([&isWiFiConnected](bool c)
                          { isWiFiConnected = c; });

        while (!isWiFiConnected)
            vTaskDelay(500 / portTICK_PERIOD_MS);

        AsyncWebServer server(80);
        ElegantOTA.begin(&server);
        ElegantOTA.setAutoReboot(true);
        WebSerial.begin(&server);
        server.begin();

        MQTTModule mqtt(
            GlobalConfig::usernameMqtt,
            GlobalConfig::passwordMqtt,
            GlobalConfig::brokerMqtt);
        MqttPayload payload;
        bool isMQTTConnected = mqtt.connect();

        while (1)
        {
            static uint8_t counter = 0;
            if (WiFi.status() != WL_CONNECTED)
            {
                isWiFiConnected = false;
                counter++;
                if (counter == 20)
                {
                    ctx->wifi.disconnect();
                    ctx->wifi.beginNB([&isWiFiConnected](bool c)
                                      { isWiFiConnected = c; });
                }
            }
            else
            {
                isWiFiConnected = true;
                counter = 0;

                ElegantOTA.loop();
                WebSerial.loop();

                if (mqtt.reconnect())
                {
                    isMQTTConnected = true;
                    if (xQueueReceive(queueSensorPublish, &payload, 0) == pdPASS)
                        mqtt.publish(payload.topic, payload.message);
                }
                else
                    isMQTTConnected = false;
            }

            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }

    static void daemonTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        SensorObject current;
        while (1)
        {
            if (globalSensorStore.getSnapshot(current))
            {
                if (current.battery < ctx->batteryProfile->getBottomSet()) // Low on battery
                {
                    vTaskSuspend(mainTaskHandle);
                    vTaskSuspend(samplingTaskHandle);
                    vTaskSuspend(notifierTaskHandle);
                }
                else if (current.battery > ctx->batteryProfile->getTopSet()) // Safe to resume
                {
                    vTaskResume(mainTaskHandle);
                    vTaskResume(samplingTaskHandle);
                    vTaskResume(notifierTaskHandle);
                }

                if (!ctx->feature.isMQTTEnabled)
                    vTaskSuspend(mqttThreadTaskHandle);
                else if (ctx->feature.isMQTTEnabled)
                    vTaskResume(mqttThreadTaskHandle);
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    static void sensorAgregatorTask(void *pvParam)
    {
        SensorContext *ctx = static_cast<SensorContext *>(pvParam);
        SensorObject sensor;
        SensorObject sensorDatapoints[32];
        while (1)
        {
            if (xQueueReceive(queueSensorAgregator, &sensor, pdMS_TO_TICKS(0)))
            {
                static uint8_t index = 0;
                sensorDatapoints[index] = sensor;

                if (index == 31)
                    index = 0;

                static uint32_t prevQueueMqtt = 0;
                if (millis() - prevQueueMqtt > 60000)
                {
                    float avgTurbidity, avgAwlr, avgBattery;
                    for (uint8_t i = 0; i < 32; i++)
                    {
                        avgTurbidity += sensorDatapoints[i].turbidity;
                        avgAwlr += sensorDatapoints[i].awlr;
                        avgBattery += sensorDatapoints[i].battery;
                    }
                    SensorObject avgSensor = SensorObject{
                        .turbidity = avgTurbidity /= 32,
                        .awlr = avgAwlr /= 32,
                        .battery = avgBattery /= 32,
                    };
                    xQueueSend(queueSensorPublish, &avgSensor, pdMS_TO_TICKS(10));
                }
            }
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    static void samplingTask(void *pvParam)
    {
        SensorContext *ctx = static_cast<SensorContext *>(pvParam);
        while (1)
        {
            ReadResult resTurbidity = ctx->turbidity.rawRead();
            ReadResult resAwlr = ctx->awlr.rawRead();
            float batteryRead = analogRead(A10);

            float turbidity, waterLevel;
            if (resTurbidity.isOk())
                turbidity = resTurbidity.value;
            if (resAwlr.isOk())
                waterLevel = resAwlr.value;

            static SensorObject sensor = SensorObject{
                .turbidity = turbidity,
                .awlr = waterLevel,
                .battery = batteryRead};
            static MqttPayload payload = MqttPayload{
                .topic = GlobalConfig::SENSOR_TOPIC,
                .message = sensor.toJson(),
            };

            xQueueSend(queueSensorAgregator, &sensor, pdTICKS_TO_MS(20));

            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    static void calibrateTask(void *pvParam)
    {
        const size_t MAX_WINDOW = 30;
        uint8_t counter = 0;
        uint16_t val[MAX_WINDOW];

        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        while (1)
        {
            if (ctx->state == AppState::CALIBRATE_TURBIDITY)
            {
                val[counter] = ctx->mbTurbidity->read();
                counter++;
            }
            else if (ctx->state == AppState::CALIBRATE_ARR)
            {
                val[counter] = ctx->mbAwlr->read();
                counter++;
            }

            if (counter >= 30)
            {
                Serial.print("array = [");
                for (uint8_t i = 0; i < MAX_WINDOW; i++)
                    Serial.printf("%d, ", val[i]);
                Serial.println("]");
                counter = 0;
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }

    static void notifierTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        MqttPayload payload;
        while (1)
        {
            payload.topic = "/test";
            payload.message = "Battery Level Warning";
            xQueueSend(queueSensorPublish, &payload, pdMS_TO_TICKS(10));

            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
