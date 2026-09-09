#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ElegantOTA.h>
#include <WebSerial.h>

#include "../consts/sensors.h"
#include "../models/shared_modbus_obj.h"
#include "../models/sensor_datastore.h"

SensorDataStore globalSensorStore;

static TaskHandle_t mainTaskHandle;
static TaskHandle_t samplingTaskHandle;
static TaskHandle_t calibrateHandle;
static TaskHandle_t notifierTaskHandle;
static TaskHandle_t mqttThreadTaskHandle;

static QueueHandle_t _publishMqttQueue;

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
        _publishMqttQueue = xQueueCreate(10, sizeof(MqttPayload));
    }

    static void daemonTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        SensorSnapshot current;
        while (1)
        {
            ElegantOTA.loop();
            WebSerial.loop();

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

    static void mqttThreadTask(void *pvParam)
    {
        MQTTModule mqtt(
            GlobalConfig::usernameMqtt,
            GlobalConfig::passwordMqtt,
            GlobalConfig::brokerMqtt);

        MqttPayload payload;
        mqtt.connect();
        while (1)
        {
            mqtt.reconnect();
            if (xQueueReceive(_publishMqttQueue, &payload, 0) == pdPASS)
                mqtt.publish(payload.topic, payload.message);

            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    static void publisherTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        SensorSnapshot snapshot;
        while (1)
        {
            if (ctx->state == AppState::NORMAL && globalSensorStore.getSnapshot(snapshot))
            {
                if (ctx->feature.isMQTTEnabled)
                {
                    char buffer[128];
                    snprintf(buffer, sizeof(buffer),
                             "Turbidity: %.1f | AWLR: %.1f",
                             snapshot.turbidity, snapshot.awlr);
                    MqttPayload payload{"/test", buffer};
                    xQueueSend(_publishMqttQueue, &payload, pdMS_TO_TICKS(10));
                }
            }

            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

    static void samplingTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        while (1)
        {
            if (ctx->state == AppState::NORMAL)
            {
                float turbidityRead = ctx->mbTurbidity->read();
                float awlrRead = ctx->mbAwlr->read();

                if (globalSensorStore.update(turbidityRead, awlrRead))
                {
                }
            }
            vTaskDelay(200 / portTICK_PERIOD_MS);
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
            xQueueSend(_publishMqttQueue, &payload, pdMS_TO_TICKS(10));

            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
