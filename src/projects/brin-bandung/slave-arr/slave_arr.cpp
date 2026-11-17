#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ModbusServerTCPasync.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Wire.h>
#include <WireGuard-ESP32.h>
#include <DFRobot_RainfallSensor.h>

#include "../config.h" // .env
#include "../utils/ntp_service.h"
#include "services/application.h"
#include "transmitter/configs/wifi_module.h"

#define DEVICE_ID 0

const char *ssid = "NodeSensorWiFi1";
const char *password = "muhammadnabiyullah";

void setup()
{
    Serial.begin(115200);

    static char hostname[64];
    snprintf(hostname, sizeof(hostname), "TFT-ARR-SLAVE-%d", DEVICE_ID + 1);
    static WiFiModule wifi(ssid, password, hostname, WIFI_POWER_19_5dBm);
    static contextDaemon daemonCtx(wifi);

    Application::init();
    configTime(7 * 3600, 0, nullptr, nullptr, nullptr);

    static contextRainfall rainCtx(Wire);
    static contextMB mbCtx(Serial1);
    xTaskCreate(Application::taskReadRainfall, "sampling WD task", 4092, &rainCtx, 3, &handleReadRainfall);
    xTaskCreate(Application::taskMBSlave, "modbus slave task", 4092, &mbCtx, 2, &handleMBSlave);
    xTaskCreate(Application::taskPollOta, "ota task", 4096, &daemonCtx, 2, &handleOta);
    xTaskCreate(Application::taskDaemon, "daemon", 4096, &daemonCtx, 1, &handleDaemon);
}

void loop() { vTaskDelay(portMAX_DELAY); }

// /*!
//  * @file  readData.ino
//  * @brief  This example describes the method of using this module to test the collected rainfall within one hour.
//  * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
//  * @license    The MIT License (MIT)
//  * @author     [fary](feng.yang@dfrobot.com)
//  * @version    V1.0
//  * @date       2023-02-28
//  * @url        https://github.com/DFRobot/DFRobot_RainfallSensor
//  */
// #include "DFRobot_RainfallSensor.h"

// // #define MODE_UART
// DFRobot_RainfallSensor_I2C Sensor(&Wire);

// void setup(void)
// {
//     Serial.begin(115200);
//     Wire.begin(1, 0);
//     delay(1000);
//     while (!Sensor.begin())
//     {
//         Serial.println("Sensor init err!!!");
//         delay(1000);
//     }
//     Serial.print("vid:\t");
//     Serial.println(Sensor.vid, HEX);
//     Serial.print("pid:\t");
//     Serial.println(Sensor.pid, HEX);
//     Serial.print("Version:\t");
//     Serial.println(Sensor.getFirmwareVersion());
//     // Set the cumulative rainfall value in units of mm.
//     // Sensor.setRainAccumulatedValue(0.2794);
// }

// void loop()
// {
//     // Get the sensor operating time in units of hours.
//     Serial.print("Sensor WorkingTime:\t");
//     Serial.print(Sensor.getSensorWorkingTime());
//     Serial.println(" H");
//     // Get the cumulative rainfall during the sensor operating time.
//     Serial.print("Rainfall:\t");
//     Serial.println(Sensor.getRainfall());
//     // Here is an example function that calculates the cumulative rainfall in a specified hour of the system. The function takes an optional argument, which can be any value between 1 and 24.
//     Serial.print("1 Hour Rainfall:\t");
//     Serial.print(Sensor.getRainfall(1));
//     Serial.println(" mm");
//     // Get the raw data, which is the tipping bucket count of rainfall, in units of counts.
//     Serial.print("rainfall raw:\t");
//     Serial.println(Sensor.getRawData());
//     delay(1000);
// }