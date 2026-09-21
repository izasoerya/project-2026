#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <ModbusClientRTU.h>
#include <RTUutils.h>
#include <TFT_eSPI.h>
#include <functional>
#include <display/display_tft_spi_lcd/display_tft.h>
#include "../../include/sensor/filters/moving_average.h"
#include "../../include/transmitter/configs/wifi_module.h"
#include "../datastore/sensor_datastore.h"
extern QueueHandle_t queueSensorRainfall;
extern QueueHandle_t queueSensorWS;
#include "../models/task_context.h"
#include "../utils/parser.h"
#include "../services/command_parser.h"
#include <projects/brin-bandung/slave-arr/utils/modbus_utility.h>

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
                Command resultParse = CommandParser::parseIncoming(data, len);
                Serial.printf("[INFO] Command received: device=%d cmd=%d payload=%lu\n",
                              resultParse.device,
                              resultParse.cmd,
                              resultParse.payload);
                if (resultParse.device == DeviceType::SLAVE_WS)
                {
                    if (resultParse.cmd == CommandType::SET_OTA)
                        ctx->modbusDataWS[6] = resultParse.payload;
                    else if (resultParse.cmd == CommandType::SET_DELAY)
                        ctx->modbusDataWS[5] = resultParse.payload;
                    else if (resultParse.cmd == CommandType::RESTART_DEVICE)
                        ctx->modbusDataWS[7] = resultParse.payload;
                }
                else if (resultParse.device == DeviceType::SLAVE_ARR)
                {
                    if (resultParse.cmd == CommandType::SET_OTA)
                        ctx->modbusDataARR[6] = resultParse.payload;
                    else if (resultParse.cmd == CommandType::SET_DELAY)
                        ctx->modbusDataARR[5] = resultParse.payload;
                    else if (resultParse.cmd == CommandType::RESTART_DEVICE)
                        ctx->modbusDataARR[7] = resultParse.payload;
                }
                else if (resultParse.device == DeviceType::MASTER)
                {
                    if (resultParse.cmd == CommandType::SET_DELAY)
                        ctx->delay->delay = resultParse.payload;
                    else if (resultParse.cmd == CommandType::RESTART_DEVICE)
                        esp_restart();
                }
            });
        server.begin();

        while (ctx->getNTPStatus() == FeatureStatus::NTP)
            vTaskDelay(500 / portTICK_PERIOD_MS);
        WireGuard wg;
        WireGuardConfig wgConfig = wgConfigs[0]; // TODO: CHANGE BASED ON SETUP
        IPAddress wgLocalIP;
        wgLocalIP.fromString(wgConfig.master.localIp);
        String wgIp = wgLocalIP.toString();
        Serial.printf("wg ip: %s\n", wgIp.c_str());
        bool wgOk = wg.begin(wgLocalIP, wgConfig.master.privateKey,
                             WG_SERVER_PUBLIC_IP, WG_SERVER_PUBLIC_KEY, WG_ENDPOINT_PORT);
        if (!wgOk)
            ctx->setWireGuardStatus(FeatureStatus::WIREGUARD);

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

            Serial.printf("ARR: %u | MB: %u | DISPLAY: %u | DAEMON: %u | SUPA: %u | OTA: %u\n",
                          uxTaskGetStackHighWaterMark(handleReadRainfall),
                          uxTaskGetStackHighWaterMark(handleMBSlave),
                          uxTaskGetStackHighWaterMark(handleDisplay),
                          uxTaskGetStackHighWaterMark(handleDaemon),
                          uxTaskGetStackHighWaterMark(handlePublish),
                          uxTaskGetStackHighWaterMark(handleOta));
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadRainfall(void *pvParam)
    {
        contextMBRainfall *ctx = static_cast<contextMBRainfall *>(pvParam);
        uint32_t stampMBCounter = 0;
        uint16_t oldStateOTA = ctx->modbusData[6];

        ctx->sharedClient.rainfallDataHandler = [ctx](ModbusMessage response, uint32_t token)
        { ctx->onDataIncoming(response, token); };
        ctx->sharedClient.rainfallErrorHandler = [ctx](Error error, uint32_t token)
        { ctx->onErrorHandler(error, token); };
        ctx->sharedClient.mb.setTimeout(10000);
        ctx->sharedClient.begin();
        while (1)
        {
            Error err = ctx->sharedClient.addRequest((uint32_t)(stampMBCounter << 1),
                                                     2, READ_HOLD_REGISTER, 0, 8);
            if (err != SUCCESS)
            {
                // TODO: HANDLE IF READ MODBUS ERROR
                String errorText = String(err);
                Serial.printf("[ERROR] Rain Modbus request: %s\n", errorText.c_str());
            }
            else
            {
                Serial.printf("[INFO] Rain Modbus FC03 request queued, token: %lu\n", stampMBCounter);
                stampMBCounter++;
            }

            if (oldStateOTA != ctx->modbusData[6])
            {
                Error errorOTA = ctx->sharedClient.addRequest((uint32_t)(stampMBCounter << 1),
                                                              2, WRITE_HOLD_REGISTER, 6, ctx->modbusData[6]);
                String errorText = String(errorOTA);
                Serial.printf("[INFO] Rain FC06 OTA write: value=%u result=%s\n",
                              ctx->modbusData[6], errorText.c_str());
                WebSerial.printf("[INFO] Rain FC06 OTA write: value=%u result=%s\n",
                                 ctx->modbusData[6], errorText.c_str());
                stampMBCounter++;
                oldStateOTA = ctx->modbusData[6];
            }

            time_t now;
            uint16_t highWord, lowWord;
            ModbusUtility::encodeUint32(static_cast<uint32_t>(time(&now)), highWord, lowWord);
            uint16_t timestamp[] = {highWord, lowWord};
            Error timestampError = ctx->sharedClient.addRequests(
                (uint32_t)(stampMBCounter << 1),
                2, WRITE_MULT_REGISTERS, 8, 2, sizeof(timestamp), timestamp);
            stampMBCounter++;

            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

    static void taskReadWS(void *pvParam)
    {
        contextMBWS *ctx = static_cast<contextMBWS *>(pvParam);
        uint32_t stampMBCounter = 0;
        uint16_t oldStateOTA = ctx->modbusData[6];

        ctx->sharedClient.wsDataHandler = [ctx](ModbusMessage response, uint32_t token)
        { ctx->onDataIncoming(response, token); };
        ctx->sharedClient.wsErrorHandler = [ctx](Error error, uint32_t token)
        { ctx->onErrorHandler(error, token); };
        ctx->sharedClient.mb.setTimeout(10000);
        ctx->sharedClient.begin();
        while (1)
        {
            Error err = ctx->sharedClient.addRequest((uint32_t)((stampMBCounter << 1) | 1),
                                                     1, READ_HOLD_REGISTER, 0, 8);
            if (err != SUCCESS)
            {
                // TODO: HANDLE IF READ MODBUS ERROR
                String errorText = String(err);
                Serial.printf("[ERROR] WS Modbus request: %s\n", errorText.c_str());
            }
            else
            {
                Serial.printf("[INFO] WS Modbus FC03 request queued, token: %lu\n", stampMBCounter);
                stampMBCounter++;
            }

            if (oldStateOTA != ctx->modbusData[6])
            {
                Error errorOTA = ctx->sharedClient.addRequest((uint32_t)(stampMBCounter << 1),
                                                              1, WRITE_HOLD_REGISTER, 6, ctx->modbusData[6]);
                String errorText = String(errorOTA);
                Serial.printf("[INFO] WS FC06 OTA write: value=%u result=%s\n",
                              ctx->modbusData[6], errorText.c_str());
                WebSerial.printf("[INFO] WS FC06 OTA write: value=%u result=%s\n",
                                 ctx->modbusData[6], errorText.c_str());
                stampMBCounter++;
                oldStateOTA = ctx->modbusData[6];
            }

            time_t now;
            uint16_t highWord, lowWord;
            ModbusUtility::encodeUint32(static_cast<uint32_t>(time(&now)), highWord, lowWord);
            uint16_t timestamp[] = {highWord, lowWord};
            Error timestampError = ctx->sharedClient.addRequests(
                (uint32_t)(stampMBCounter << 1),
                1, WRITE_MULT_REGISTERS, 8, 2, sizeof(timestamp), timestamp);
            stampMBCounter++;

            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

    struct SensorDatapoint
    {
        float temperature = 0;
        float humidity = 0;
        float windSpeed = 0;
        float rainfall = 0;
    };

    static void taskSendSupabase(void *pvParam)
    {
        contextPublisher *ctx = static_cast<contextPublisher *>(pvParam);
        static SensorWSObject ws;
        static SensorRainfallObject rain;
        static SensorPublishableObject sensor{};

        static constexpr uint8_t MAX_DATAPOINTS = 30;
        static SensorDatapoint dataBuffer[MAX_DATAPOINTS] = {};
        static uint8_t bufferIndex = 0;
        static uint8_t pointsCollected = 0;
        static uint32_t lastStoreTime = 0;
        static uint32_t lastSendTime = 0;

        while (1)
        {
            bool sensorUpdated = false;

            if (xQueueReceive(queueSensorWS, &ws, pdMS_TO_TICKS(0)) == pdPASS)
            {
                sensor.temperature = ws.temperature;
                sensor.humidity = ws.humidity;
                if (sensor.windSpeed < 1.5)
                    sensor.windSpeed = 0;
                else
                    sensor.windSpeed = ws.windSpeed;
                sensor.windDirection = ws.windDirection;
                sensorUpdated = true;
            }

            if (xQueueReceive(queueSensorRainfall, &rain, pdMS_TO_TICKS(0)) == pdPASS)
            {
                sensor.rainfall = rain.rainfall;
                sensorUpdated = true;
            }

            if (sensorUpdated)
            {
                if (singletonSensorFull.update(sensor))
                    xQueueSend(queueSensorDashboard, &sensor, pdMS_TO_TICKS(10));
            }

            if (millis() - lastStoreTime >= 30000)
            {
                lastStoreTime = millis();
                dataBuffer[bufferIndex].temperature = sensor.temperature;
                dataBuffer[bufferIndex].humidity = sensor.humidity;
                dataBuffer[bufferIndex].windSpeed = sensor.windSpeed;
                dataBuffer[bufferIndex].rainfall = sensor.rainfall;
                bufferIndex = (bufferIndex + 1) % MAX_DATAPOINTS;

                if (pointsCollected < MAX_DATAPOINTS)
                    pointsCollected++;
            }

            if (millis() - lastSendTime >= ctx->delay->delay && pointsCollected > 0)
            {
                lastSendTime = millis();
                float avgTemp = 0, avgHumidity = 0, avgWindSpeed = 0, sumRainfall = 0;
                for (uint8_t i = 0; i < pointsCollected; i++)
                {
                    avgTemp += dataBuffer[i].temperature;
                    avgHumidity += dataBuffer[i].humidity;
                    avgWindSpeed += dataBuffer[i].windSpeed;
                    sumRainfall += dataBuffer[i].rainfall;
                }
                avgTemp /= pointsCollected;
                avgHumidity /= pointsCollected;
                avgWindSpeed /= pointsCollected;

                sensor.temperature = avgTemp;
                sensor.humidity = avgHumidity;
                sensor.windSpeed = avgWindSpeed;
                sensor.rainfall = sumRainfall;

                ctx->wifi.setTransport(&ctx->transport);
                char jsonBuffer[256];
                sensor.toJson(jsonBuffer, sizeof(jsonBuffer));
                int16_t response = ctx->wifi.send(ctx->tableSensor, jsonBuffer);

                Serial.printf("[INFO] POST Supa (avg of %u): %d\n", pointsCollected, response);
                WebSerial.printf("[INFO] POST Supa (avg of %u): %d\n", pointsCollected, response);

                memset(dataBuffer, 0, sizeof(dataBuffer));
                bufferIndex = 0;
                pointsCollected = 0;
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
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
            display.setSignalStrength(WiFi.RSSI());
            display.refresh();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
