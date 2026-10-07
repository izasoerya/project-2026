#include <Arduino.h>
#include <Preferences.h>

#include "consts/global_config.h"
#include "consts/sensors.h"
#include "models/profile.h"
#include "models/task_context.h"
#include "services/fsm_kernel.h"
#include "transmitter/configs/wifi_module.h"

void setup()
{
    esp_sleep_enable_timer_wakeup(GlobalConfig::DELAY_SAMPLING * 1000ULL); // Configure sleep for 30s
    Preferences prefs;
    prefs.begin("app_config", true);
    const uint8_t firmwareId = prefs.getUChar("id", 0);
    prefs.end();
    if (firmwareId == 0)
    {
        prefs.begin("app_config", false);
        prefs.putUChar("id", GlobalConfig::deviceID);
        prefs.end();
        esp_restart();
    }

    Serial.begin(115200);
    Serial.printf("Firmware ID: %u\n", firmwareId);
    GlobalConfig::mbSerial.begin(9600, SERIAL_8N1, 20, 21);

    const float batteryDangerLevel = 11.75;
    const float batteryWarningLevel = 12.0;
    Profile batteryProfileDefault(batteryDangerLevel, batteryWarningLevel);

    static Modbustatics *turbSensor = nullptr;
    static Modbustatics *awlrSensor = nullptr;
    for (int i = 0; i < sizeof(turb_list); i++)
    {
        if (turb_list[i].getId() == firmwareId)
            turbSensor = &turb_list[i];
        if (awlr_list[i].getId() == firmwareId)
            awlrSensor = &awlr_list[i];
    }
    static SensorContext sensorCtx(*turbSensor, *awlrSensor);

    static WiFiModule inet(
        GlobalConfig::ssid, GlobalConfig::password, GlobalConfig::hostname,
        wifi_power_t::WIFI_POWER_19_5dBm);
    static NetworkingContext networkCtx(inet);

    FSMKernel kernel = FSMKernel(&sensorCtx, &networkCtx);
    while (1)
        kernel.loop();
}

void loop() {}