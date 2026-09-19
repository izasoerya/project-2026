#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ModbusServerRTU.h>
#include "../../include/sensor/filters/moving_average.h"
#include "../models/task_context.h"
#include "../datastore/sensor_datastore.h"
#include "../utils/parser.h"
#include "nvs_manager.h"
#include <projects/brin-bandung/slave-arr/utils/enum.h>

TaskHandle_t handleReadRainfall;
TaskHandle_t handleMBSlave;
TaskHandle_t handleDaemon;
TaskHandle_t handleOta;

SensorDatastore singletonSensor;
QueueHandle_t queueSensor;

class Application
{
public:
    static void init()
    {
        queueSensor = xQueueCreate(10, sizeof(SensorObject));
        NVSManager::ensureRainfall();
    }

    static void taskPollOta(void *pvParam)
    {
        contextDaemon *ctx = static_cast<contextDaemon *>(pvParam);
        vTaskSuspend(NULL);

        static bool isWiFiConnected = false;
        Serial.println("Connecting to WiFi");
        ctx->wifi.beginNB([](bool t)
                          { isWiFiConnected = t; });

        while (!isWiFiConnected)
            vTaskDelay(500 / portTICK_PERIOD_MS);

        Serial.printf("Connected with IP: %s\n", ctx->wifi.localIP());
        if (NTPService::init())
            ctx->setNTPStatus(FeatureStatus::WORKING);

        // WireGuard wg;
        // WireGuardConfig wgConfig = wgConfigs[0]; // TODO: CHANGE BASED ON SETUP
        // IPAddress wgLocalIP;
        // wgLocalIP.fromString(wgConfig.master.localIp);
        // Serial.printf("wg ip: %s\n", wgLocalIP.toString());
        // bool wgOk = wg.begin(wgLocalIP, wgConfig.master.privateKey,
        //                      WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
        // if (!wgOk)
        //     ctx->setWireGuardStatus(FeatureStatus::WIREGUARD);

        AsyncWebServer server(80);
        ElegantOTA.begin(&server);
        ElegantOTA.setAutoReboot(true);
        WebSerial.begin(&server);
        server.begin();

        static uint16_t counter = 0;
        while (1)
        {
            ElegantOTA.loop();
            WebSerial.loop();

            if (WiFi.status() != WL_CONNECTED)
            {
                counter++;
                if (counter > 1000)
                {
                    counter = 0;
                    Serial.printf("Reconnecting...\n");
                    ctx->wifi.disconnect();
                    ctx->wifi.beginNB();
                }
            }
            else
                counter = 0;

            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }

    static void taskDaemon(void *pvParam)
    {
        contextDaemon *ctx = static_cast<contextDaemon *>(pvParam);
        static uint32_t prevReconnect = millis();
        static uint32_t prevCheckNTP = millis();

        while (1)
        {
            if (ctx->modbusData[5] == 1)
                ctx->enabledFeature[0] = Feature::INTERNET_FEAUTRE;
            if (ctx->modbusData[6] == 1)
                ctx->enabledFeature[1] = Feature::OTA_FEATURE;
            else if (ctx->modbusData[6] == 0)
                ctx->enabledFeature[1] = Feature::FEATURE_DISABLED;

            if (ctx->enabledFeature[0] == Feature::INTERNET_FEAUTRE)
                if (WiFi.status() != WL_CONNECTED)
                {
                    static uint32_t prevConnect = 0;
                    if (millis() - prevConnect > 15000) // Timeout on 15 second
                    {
                        prevConnect = millis();
                        ctx->wifi.begin([]() {}, []() {});
                    }
                    ctx->setInternetStatus(FeatureStatus::INTERNET);
                }
                else if (WiFi.status() == WL_CONNECTED)
                    ctx->setInternetStatus(FeatureStatus::WORKING);

            if (ctx->enabledFeature[1] == Feature::OTA_FEATURE)
                vTaskResume(handleOta);
            else if (ctx->enabledFeature[1] == Feature::FEATURE_DISABLED)
            {
                vTaskSuspend(handleOta);
                ctx->wifi.disconnect();
            }

            Serial.printf("Rain: %u | MB: %u | Daemon: %u | OTA: %u\n",
                          uxTaskGetStackHighWaterMark(handleReadRainfall),
                          uxTaskGetStackHighWaterMark(handleMBSlave),
                          uxTaskGetStackHighWaterMark(handleDaemon),
                          uxTaskGetStackHighWaterMark(handleOta));

            Serial.printf("TEST NVS: %.1f\n", NVSManager::getTest());

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadRainfall(void *pvParam)
    {
        contextRainfall *ctx = static_cast<contextRainfall *>(pvParam);
        DFRobot_RainfallSensor_I2C rainSensor(&ctx->wire);
        bool resetDoneToday = false;

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
            Serial.printf("Failed to begin I2C Sensor");
        }

        while (1)
        {
            float rain = rainSensor.getRainfall(24);
            SensorObject snapshot = SensorObject{
                .rainfall = rain,
            };
            if (singletonSensor.update(snapshot))
            {
                xQueueSend(queueSensor, &snapshot, pdMS_TO_TICKS(10));
                Serial.println("[INFO] Success to send sensor datastore to queue");

                float lastRainfallValue = NVSManager::getRainfall();
                if (lastRainfallValue != snapshot.rainfall)
                    NVSManager::storeRainfall(snapshot.rainfall);
            }
            else
                Serial.println("Failed to update sensor datastore");

            TimeStruct ts = NTPService::getTime();
            if (ts.hour == 0 && ts.minute == 0 && !resetDoneToday)
            {
                rainSensor.setRainAccumulatedValue(0);
                NVSManager::storeRainfall(0.0F);
                resetDoneToday = true;
            }
            else if (ts.hour != 0 || ts.minute != 0)
                resetDoneToday = false;

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskMBSlave(void *pvParam)
    {
        contextMB *mbCtx = static_cast<contextMB *>(pvParam);
        ModbusServerRTU mbServer(2000); // Timeout 2000ms
        SensorObject payload;

        mbCtx->serial.begin(9600, SERIAL_8N1, mbCtx->pinRX, mbCtx->pinTX);
        mbServer.registerWorker(0x01, READ_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC03(request); });
        mbServer.registerWorker(0x01, WRITE_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC06(request); });
        mbServer.begin(mbCtx->serial);
        while (1)
        {
            if (xQueueReceive(queueSensor, &payload, 0) == pdPASS)
            {
                mbCtx->modbusData[0] = uint16_t(payload.rainfall * 10);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
