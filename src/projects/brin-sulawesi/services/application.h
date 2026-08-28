#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ElegantOTA.h>
#include <WebSerial.h>

#include "../consts/sensors.h"
#include "../models/shared_modbus_obj.h"

class Application
{
private:
    static QueueHandle_t _turbidityQueue;
    static QueueHandle_t _awlrQueue;

public:
    static void initTask()
    {
        _turbidityQueue = xQueueCreate(10, sizeof(float));
        _awlrQueue = xQueueCreate(10, sizeof(float));
    }

    static void mainTask(void *pvParam)
    {
        float receivedTurbidity = 0;
        float receivedAwlr = 0;

        TickType_t prevLog = xTaskGetTickCount();
        while (1)
        {
            ElegantOTA.loop();
            WebSerial.loop();

            ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
            if (ctx->state == AppState::NORMAL)
            {
                if (xQueueReceive(_turbidityQueue, &receivedTurbidity, 0) == pdPASS)
                {
                }

                if (xQueueReceive(_awlrQueue, &receivedAwlr, 0) == pdPASS)
                {
                }

                ctx->mqtt->reconnect();

                if ((xTaskGetTickCount() - prevLog) >= pdMS_TO_TICKS(10 * 1000))
                {
                    prevLog = xTaskGetTickCount();

                    char buffer[128];
                    snprintf(buffer, sizeof(buffer),
                             "Turbidity: %.1f | AWLR: %.1f",
                             receivedTurbidity, receivedAwlr);
                    if (ctx->feature.isMQTTEnabled)
                    {
                        uint16_t res = ctx->mqtt->publish("/test", buffer);
                        if (res == 0)
                        {
                            Serial.println("MQTT Publish Failed");
                            WebSerial.println("MQTT Publish Failed");
                        }
                    }
                    Serial.printf("Success Publish: %s\n", buffer);
                    WebSerial.printf("Success Publish: %s\n", buffer);
                }

                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
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

                xQueueSend(_turbidityQueue, &turbidityRead, pdMS_TO_TICKS(10));
                xQueueSend(_awlrQueue, &awlrRead, pdMS_TO_TICKS(10));
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
};

#endif // APPLICATION_H
