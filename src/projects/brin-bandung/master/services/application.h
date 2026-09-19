#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ModbusClientRTU.h>
#include <RTUutils.h>
#include <TFT_eSPI.h>
#include <functional>
#include <display/display_tft_spi_lcd/display_tft.h>
#include "../../include/sensor/filters/moving_average.h"
#include "../../include/transmitter/configs/wifi_module.h"
#include "../models/task_context.h"
#include "../datastore/sensor_datastore.h"
#include "../utils/parser.h"
#include "../services/command_parser.h"

TaskHandle_t handleReadRainfall;
TaskHandle_t handleMBSlave;
TaskHandle_t handleDaemon;
TaskHandle_t handleDisplay;
TaskHandle_t handlePublish;
TaskHandle_t handleOta;

SensorRainfallDatastore singletonSensorRainfall;
SensorWSDatastore singletonSensorWS;
SensorPublishableDatastore singletonSensorFull;
QueueHandle_t queueSensorRainfall;
QueueHandle_t queueSensorWS;
QueueHandle_t queueSensorDashboard;

class Application
{
public:
    static void init()
    {
        queueSensorRainfall = xQueueCreate(10, sizeof(SensorRainfallObject));
        queueSensorWS = xQueueCreate(10, sizeof(SensorWSObject));
        queueSensorDashboard = xQueueCreate(10, sizeof(SensorPublishableObject));
    }

    static void taskPollOta(void *pvParam)
    {
        contextDaemon *ctx = static_cast<contextDaemon *>(pvParam);

        static bool isWiFiConnected = false;
        Serial.println("Connecting to WiFi");
        ctx->wifi.beginNB([](bool t)
                          { isWiFiConnected = t; });
        while (!isWiFiConnected)
            vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.printf("Connected with IP: %s\n", ctx->wifi.localIP());

        AsyncWebServer server(80);
        ElegantOTA.begin(&server);
        ElegantOTA.setAutoReboot(true);
        WebSerial.begin(&server);
        WebSerial.onMessage(
            [ctx](uint8_t *data, size_t len)
            {
                EnabledOTA res = CommandParser::otaCommand(data, len, ctx->modbusDataWS);
                if (static_cast<uint8_t>(res) != 0)
                {
                    if (res == EnabledOTA::SLAVE_WS_ON)
                        ctx->modbusDataWS[6] = 1;
                    else if (res == EnabledOTA::SLAVE_WS_OFF)
                        ctx->modbusDataWS[6] = 0;
                    else if (res == EnabledOTA::SLAVE_ARR_ON)
                        ctx->modbusDataARR[6] = 1;
                    else if (res == EnabledOTA::SLAVE_ARR_OFF)
                        ctx->modbusDataARR[6] = 0;
                }
                else
                {
                    ctx->modbusDataWS[5] = CommandParser::configCommand(data, len);
                }
            });
        server.begin();

        while (1)
        {
            ElegantOTA.loop();
            WebSerial.loop();

            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }

    static void taskDaemon(void *pvParam)
    {
        contextDaemon *ctx = static_cast<contextDaemon *>(pvParam);
        static uint32_t prevReconnect = millis();
        static uint32_t prevCheckNTP = millis();
        static uint32_t prevCheckInternet = millis();

        while (1)
        {
            TimeStruct ts = NTPService::getTime();
            Serial.printf("Time: %d:%d:%d\n", ts.hour, ts.minute, ts.second);

            if (ctx->getNTPStatus() == FeatureStatus::NTP && millis() - prevCheckNTP > 1000)
            {
                prevCheckNTP = millis();
                if (NTPService::init())
                    ctx->setNTPStatus(FeatureStatus::WORKING);
            }
            else
                prevCheckNTP = millis();

            if (WiFi.status() != WL_CONNECTED)
                if (millis() - prevReconnect > 20000)
                {
                    prevReconnect = millis();

                    Serial.printf("Reconnecting...\n");
                    ctx->wifi.disconnect();
                    ctx->wifi.beginNB();
                }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadRainfall(void *pvParam)
    {
        contextMBRainfall *ctx = static_cast<contextMBRainfall *>(pvParam);
        ModbusClientRTU mb;
        SensorRainfallObject sensor;
        uint32_t stampMBCounter = 0;
        uint16_t oldStateOTA = ctx->modbusData[7];

        RTUutils::prepareHardwareSerial(ctx->serial);
        ctx->serial.begin(9600, SERIAL_8N1, ctx->pinRX, ctx->pinTX);
        mb.onDataHandler([ctx](ModbusMessage response, uint32_t token)
                         { ctx->onDataIncoming(response, token); });
        mb.onErrorHandler([ctx](Error error, uint32_t token)
                          { ctx->onErrorHandler(error, token); });
        mb.setTimeout(10000);
        mb.begin(ctx->serial);
        while (1)
        {
            Error err = mb.addRequest((uint32_t)stampMBCounter, // Token
                                      1, READ_HOLD_REGISTER, 0, 1);
            if (err != SUCCESS)
            {
                // TODO: HANDLE IF READ MODBUS ERROR
                Serial.printf("[ERROR] Rain Modbus request: %s\n", String(err));
            }
            else
            {
                Serial.printf("[INFO] WS Modbus FC03 request queued, token: %lu\n", stampMBCounter);
                stampMBCounter++;

                sensor.rainfall = ctx->modbusData[4] / 10.0F;

                xQueueSend(queueSensorRainfall, &sensor, pdTICKS_TO_MS(10));
            }
            xQueueSend(queueSensorRainfall, &sensor, pdTICKS_TO_MS(10));

            if (oldStateOTA != ctx->modbusData[7])
            {
                Error err = mb.addRequest((uint32_t)stampMBCounter, // Token
                                          1, WRITE_HOLD_REGISTER, 7, ctx->modbusData[7]);
                oldStateOTA = ctx->modbusData[7];
            }

            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadWS(void *pvParam)
    {
        contextMBWS *ctx = static_cast<contextMBWS *>(pvParam);
        ModbusClientRTU mb;
        SensorWSObject sensorWS;
        SensorRainfallObject sensorRain;
        uint32_t stampMBCounter = 0;
        uint16_t oldStateOTA = ctx->modbusData[6];

        RTUutils::prepareHardwareSerial(ctx->serial);
        ctx->serial.begin(9600, SERIAL_8N1, ctx->pinRX, ctx->pinTX);
        mb.onDataHandler([ctx](ModbusMessage response, uint32_t token)
                         { ctx->onDataIncoming(response, token); });
        mb.onErrorHandler([ctx](Error error, uint32_t token)
                          { ctx->onErrorHandler(error, token); });
        mb.setTimeout(10000);
        mb.begin(ctx->serial);
        while (1)
        {
            Error err = mb.addRequest((uint32_t)stampMBCounter, // Token
                                      1, READ_HOLD_REGISTER, 0, 4);
            if (err != SUCCESS)
            {
                // TODO: HANDLE IF READ MODBUS ERROR
                Serial.printf("[ERROR] WS Modbus request: %s\n", String(err));
            }
            else
            {
                Serial.printf("[INFO] WS Modbus FC03 request queued, token: %lu\n", stampMBCounter);
                stampMBCounter++;

                sensorWS.temperature = ctx->modbusData[0] / 10.0F;
                sensorWS.humidity = ctx->modbusData[1] / 10.0F;
                sensorWS.windSpeed = ctx->modbusData[2] / 10.0F;
                sensorWS.windDirection = static_cast<WindDirectionEnum>(ctx->modbusData[3]);

                xQueueSend(queueSensorWS, &sensorWS, pdTICKS_TO_MS(10));
            }

            Serial.printf("[INFO] MB ADDR[6] = %d\n", ctx->modbusData[6]);
            Error errorOTAWS = mb.addRequest((uint32_t)stampMBCounter, // Token
                                             1, WRITE_HOLD_REGISTER, 6, ctx->modbusData[6]);

            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

    static void taskSendSupabase(void *pvParam)
    {
        contextPublisher *ctx = static_cast<contextPublisher *>(pvParam);
        static SensorWSObject ws;
        static SensorRainfallObject rain;
        static SensorPublishableObject sensor;

        while (1)
        {
            if (xQueueReceive(queueSensorRainfall, &rain, pdTICKS_TO_MS(0)) == pdPASS ||
                xQueueReceive(queueSensorWS, &ws, pdTICKS_TO_MS(0)) == pdPASS)
            {
                sensor.temperature = ws.temperature;
                sensor.humidity = ws.humidity;
                sensor.windSpeed = ws.windSpeed;
                sensor.windDirection = ws.windDirection;
                sensor.rainfall = rain.rainfall;

                if (singletonSensorFull.update(sensor))
                    xQueueSend(queueSensorDashboard, &sensor, pdMS_TO_TICKS(10));
            }

            static uint32_t lastSendTime = 0;
            if (millis() - lastSendTime >= 60000)
            {
                lastSendTime = millis();

                ctx->wifi.setTransport(&ctx->transport);
                char buffer[256];
                sensor.toJson(buffer, sizeof(buffer));
                int16_t response = ctx->wifi.send("%5BWS-DEV%5D%20sensors", buffer);

                Serial.printf("[INFO] POST Supa: %d\n", response);
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS); // Loop every 1 second
        }
    }

    static void taskDisplayDashboard(void *pvParam)
    {
        contextDisplay *ctx = static_cast<contextDisplay *>(pvParam);
        TFT_eSPI tft = TFT_eSPI();
        DisplayTFT320X480PV display(tft);
        static SensorPublishableObject sensor;

        ctx->spi.begin(ctx->pinSCK, ctx->pinMISO, ctx->pinMOSI, ctx->pinCS);
        display.begin();
        display.setHeaderTitle("WEATHER-STATION-THWDR");
        display.setFooterText("v1.0.0");
        display.setContainer1("TEMPERATURE", "0 *C", DisplayColor::ORANGE, IconType::THERMO);
        display.setContainer2("HUMIDITY", "0 %RH", DisplayColor::BLUE, IconType::DROPLET);
        display.setContainer3("WIND SPEED", "0 m/s", DisplayColor::TEXT, IconType::WIND);
        display.setContainer4("WIND DIR", "North", DisplayColor::YELLOW, IconType::COMPASS);
        display.setContainer5("RAINFALL", "0 mm/day", DisplayColor::TEAL, IconType::RAIN);
        display.setContainer6("COMPANY", "T4T x ZTS", DisplayColor::GREEN, IconType::COMPANY);
        display.drawLayout(); // one full paint of shells/borders/icons/labels

        while (1)
        {
            if (xQueueReceive(queueSensorDashboard, &sensor, pdTICKS_TO_MS(0)))
            {
                static char buf[24]; // Follow max char in custom library
                snprintf(buf, sizeof(buf), "%.1f C", sensor.temperature);
                display.updateContainerValue(1, buf);

                snprintf(buf, sizeof(buf), "%.1f %RH", sensor.humidity);
                display.updateContainerValue(2, buf);

                snprintf(buf, sizeof(buf), "%.1f km/h", sensor.windSpeed);
                display.updateContainerValue(3, buf);

                snprintf(buf, sizeof(buf), "%s", Parser::parseWindDirection(sensor.windDirection));
                display.updateContainerValue(4, buf);

                snprintf(buf, sizeof(buf), "%.1f mm/day", sensor.rainfall);
                display.updateContainerValue(5, buf);
            }

            TimeStruct ts = NTPService::getTime();
            display.setClock(ts.hour, ts.minute, ts.second);
            display.refresh();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
