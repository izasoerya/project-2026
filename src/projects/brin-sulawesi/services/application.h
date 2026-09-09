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

class Application
{
public:
    static void daemonTask(void *pvParam)
    {
        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        SensorSnapshot current;
        while (1)
        {
            ctx->mqtt->reconnect();
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
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
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
                    uint16_t res = ctx->mqtt->publish("/test", buffer);
                    if (res == 0)
                    {
                        Serial.println("MQTT Publish Failed");
                        WebSerial.println("MQTT Publish Failed");
                    }
                    Serial.printf("Success Publish: %s\n", buffer);
                    WebSerial.printf("Success Publish: %s\n", buffer);
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
        vTaskSuspend(NULL);

        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        while (1)
        {
            ctx->mqtt->publish("/test", "Battery Level Warning");
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
