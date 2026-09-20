#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ElegantOTA.h>
#include <WebSerial.h>
#include <ModbusServerRTU.h>
#include <RTUutils.h>
#include <ClosedCube_SHT31D.h>

#include "../../include/sensor/filters/moving_average.h"
#include "../models/task_context.h"
#include "../datastore/sensor_datastore.h"
#include "../utils/parser.h"
#include "../utils/modbus_utility.h"

TaskHandle_t handleReadWD = NULL;
TaskHandle_t handleReadTHWS = NULL;
TaskHandle_t handleMBSlave = NULL;
TaskHandle_t handleDaemon = NULL;
TaskHandle_t handleOta = NULL;

volatile uint32_t counterAnemo;
volatile WindDirectionEnum windDirection;

SensorDatastore singletonSensor;
QueueHandle_t queueSensorDatastore;

class Application
{
public:
    static void init()
    {
        queueSensorDatastore = xQueueCreate(10, sizeof(SensorObject));
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
            if (ctx->modbusData[6] == 1)
                ctx->enabledFeature[1] = Feature::OTA_FEATURE;
            else if (ctx->modbusData[6] == 0)
                ctx->enabledFeature[1] = Feature::FEATURE_DISABLED;
            else if (ctx->modbusData[7] == 1)
                esp_restart();

            static uint32_t lastSyncedUnix = 0;
            uint32_t unixTime = ModbusUtility::decodeUint32(
                ctx->modbusData[8], ctx->modbusData[9]);
            if (unixTime != lastSyncedUnix && unixTime > 1672531200)
            {
                NTPService::setTime(unixTime);
                lastSyncedUnix = unixTime;
                Serial.printf("[INFO] Time synced: %lu\n", unixTime);
            }

            time_t now = time(nullptr);
            bool isTimeSet = (now > 1672531200); // After Jan 1, 2023
            if (isTimeSet)
            {
                TimeStruct ts = NTPService::getTime();
                Serial.printf("Clock: %d:%d:%d\n", ts.hour, ts.minute, ts.second);
            }

            if (ctx->enabledFeature[1] == Feature::OTA_FEATURE)
                vTaskResume(handleOta);
            else if (ctx->enabledFeature[1] == Feature::FEATURE_DISABLED)
            {
                vTaskSuspend(handleOta);
                ctx->wifi.disconnect();
            }

            Serial.printf("WD: %u | THWS: %u | MB: %u | Daemon: %u | OTA: %u\n",
                          uxTaskGetStackHighWaterMark(handleReadWD),
                          uxTaskGetStackHighWaterMark(handleReadTHWS),
                          uxTaskGetStackHighWaterMark(handleMBSlave),
                          uxTaskGetStackHighWaterMark(handleDaemon),
                          uxTaskGetStackHighWaterMark(handleOta));

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadWD(void *pvParam)
    {
        contextWD *wdCtx = static_cast<contextWD *>(pvParam);
        wdCtx->serial.begin(9600, SERIAL_8N1, wdCtx->pinRX, wdCtx->pinTX);
        while (1)
        {
            String data = wdCtx->serial.readString(); // data yang diterima dari sensor berawalan tanda * dan diakhiri tanda #, contoh *1#
            int a = data.indexOf("*");                // a adalah index tanda *
            int b = data.indexOf("#");                // b adalah index tanda #
            String resultWind = data.substring(a + 1, b);
            windDirection = Parser::parseStringWindDirection(resultWind);

            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadTHWS(void *pvParam)
    {
        contextTHWS *wd = static_cast<contextTHWS *>(pvParam);
        TrimmedMovingAverage filteredTemperature(20, 4);
        TrimmedMovingAverage filteredHumidity(20, 4);
        TrimmedMovingAverage filteredAnemoter(20, 4);

        ClosedCube_SHT31D sht;
        Wire.begin(wd->pinSDA, wd->pinSCL);
        sht.begin(0x44); // I2C address can be 0x44 or 0x45
        if (sht.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) != SHT3XD_NO_ERROR)
            Serial.println("SHT INIT FAILED");

        const uint8_t pinAnemo = 9;
        pinMode(pinAnemo, INPUT_PULLUP);
        gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
        detachInterrupt(pinAnemo);
        attachInterrupt(digitalPinToInterrupt(pinAnemo), Application::anemoInterruptHandler, FALLING);
        uint32_t windSpeed;

        WindDirectionEnum windDirection;

        while (1)
        {
            SHT31D shtResult = sht.periodicFetchData();
            noInterrupts();
            uint32_t copyCounter = counterAnemo;
            counterAnemo = 0;
            interrupts();
            float rpm = float(copyCounter / (1000.0 / 1000.0));                                 // Return in rotation/minute
            float windSpeedResult = ((-0.0181 * (rpm * rpm))) + (1.3859 * float(rpm)) + 1.4055; // Return in m/s

            SensorObject snapshot = SensorObject{
                .temperature = filteredTemperature.filter(shtResult.t),
                .humidity = filteredHumidity.filter(shtResult.rh),
                .windSpeed = filteredAnemoter.filter(windSpeedResult),
                .windDirection = windDirection,
            };
            if (singletonSensor.update(snapshot))
                xQueueSend(queueSensorDatastore, &snapshot, pdMS_TO_TICKS(10));
            else
                Serial.println("Failed to send sensor datastore to queue");

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskMBSlave(void *pvParam)
    {
        contextMB *mbCtx = static_cast<contextMB *>(pvParam);
        ModbusServerRTU mbServer(2000); // Timeout 2000ms
        SensorObject payload;

        RTUutils::prepareHardwareSerial(mbCtx->serial);
        mbCtx->serial.begin(9600, SERIAL_8N1, mbCtx->pinRX, mbCtx->pinTX);
        mbServer.registerWorker(0x01, READ_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC03(request); });
        mbServer.registerWorker(0x01, WRITE_HOLD_REGISTER, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC06(request); });
        mbServer.registerWorker(0x01, WRITE_MULT_REGISTERS, [mbCtx](ModbusMessage request)
                                { return mbCtx->FC16(request); });
        mbServer.begin(mbCtx->serial);
        while (1)
        {
            if (xQueueReceive(queueSensorDatastore, &payload, 0) == pdPASS)
            {
                mbCtx->modbusData[0] = uint16_t(10);
                mbCtx->modbusData[1] = uint16_t(payload.humidity * 10);
                mbCtx->modbusData[2] = uint16_t(payload.windSpeed * 10);
                mbCtx->modbusData[3] = uint16_t(payload.windDirection);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

    static void ARDUINO_ISR_ATTR anemoInterruptHandler()
    {
        const uint16_t debounce = 5;
        static uint32_t prevDebounceAnemo = 0;

        if (millis() - prevDebounceAnemo > debounce)
        {
            prevDebounceAnemo = millis();
            counterAnemo++;
        }
    }
};

#endif // APPLICATION_H
