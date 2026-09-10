#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ModbusServerRTU.h>
#include "../../include/sensor/filters/moving_average.h"
#include "../models/task_context.h"
#include "../datastore/sensor_datastore.h"
#include "../utils/parser.h"
#include "nvs_manager.h"

TaskHandle_t handleReadRainfall;
TaskHandle_t handleMBSlave;
TaskHandle_t handleDaemon;

SensorDatastore singletonSensor;
QueueHandle_t queueSensorDatastore;

class Application
{
public:
    static void init()
    {
        queueSensorDatastore = xQueueCreate(10, sizeof(SensorDatastore));
    }

    static void taskDaemon(void *pvParam)
    {
        while (1)
        {
            TimeStruct ts = NTPService::getTime();
            Serial.printf("Time: %d:%d:%d\n", ts.hour, ts.minute, ts.second);

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadRainfall(void *pvParam)
    {
        contextRainfall *ctx = static_cast<contextRainfall *>(pvParam);
        TrimmedMovingAverage filteredRainfall(20, 4);
        DFRobot_RainfallSensor_I2C rainSensor(&ctx->wire);

        Wire.begin(ctx->pinSDA, ctx->pinSCL);
        if (rainSensor.begin())
        {
            float lastRainValue = NVSManager::getRainfall();
            if (lastRainValue != -1)
                rainSensor.setRainAccumulatedValue(lastRainValue);
        }
        else
        {
            // TODO: HANDLE IF RAINFALL SENSOR FAIL
        }

        while (1)
        {
            float rain = rainSensor.getRainfall(24);
            SensorObject snapshot = SensorObject{
                .rainfall = filteredRainfall.filter(rain),
            };
            if (singletonSensor.update(snapshot))
            {
                xQueueSend(queueSensorDatastore, &singletonSensor, pdMS_TO_TICKS(10));
                Serial.println("Success to send sensor datastore to queue");

                float lastRainfallValue = NVSManager::getRainfall();
                if (lastRainfallValue != snapshot.rainfall)
                    NVSManager::storeRainfall(snapshot.rainfall);
            }
            else
            {
                Serial.println("Failed to send sensor datastore to queue");
                // TODO: HANDLING WHEN PUSH QUEUE FAILING
            }

            TimeStruct ts = NTPService::getTime();
            if (ts.hour == 0 && ts.minute == 0)
                rainSensor.setRainAccumulatedValue(0);

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskMBSlave(void *pvParam)
    {
        contextMB *mbCtx = static_cast<contextMB *>(pvParam);
        ModbusServerRTU mbServer(2000); // Timeout 2000ms
        SensorObject payload;

        Serial1.begin(9600, SERIAL_8N1, mbCtx->pinRX, mbCtx->pinTX);
        mbServer.registerWorker(0x01, READ_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC03(request); });
        mbServer.registerWorker(0x01, WRITE_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC06(request); });
        mbServer.begin(mbCtx->serial);
        while (1)
        {
            if (xQueueReceive(queueSensorDatastore, &payload, 0) == pdPASS)
            {
                mbCtx->data[0] = uint16_t(payload.rainfall * 10);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
