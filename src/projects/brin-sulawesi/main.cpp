#include <Preferences.h>

#include "consts/global_config.h"
#include "consts/sensors.h"
#include "models/profile.h"
#include "models/task_context.h"
#include "services/fsm_kernel.h"
#include "transmitter/configs/wifi_module.h"

WiFiModule inet(
    GlobalConfig::ssid,
    GlobalConfig::password,
    GlobalConfig::hostname,
    WIFI_POWER_19_5dBm);

Modbustatics *turbSensor = nullptr;
Modbustatics *awlrSensor = nullptr;

FSMKernel *kernel = nullptr;

void setup()
{
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

    esp_sleep_enable_timer_wakeup(30 * 1000000ULL); // Sleep for 30s

    for (int i = 0; i < sizeof(turb_list) / sizeof(Modbustatics); i++)
    {
        if (turb_list[i].getId() == firmwareId)
            turbSensor = &turb_list[i];
        if (awlr_list[i].getId() == firmwareId)
            awlrSensor = &awlr_list[i];
    }

    const float batteryDangerLevel = 11.75;
    const float batteryWarningLevel = 12.0;
    Profile batteryProfileDefault(batteryDangerLevel, batteryWarningLevel);

    static NetworkingContext networkCtx(inet);
    static SensorContext sensorCtx(*turbSensor, *awlrSensor);

    kernel = new FSMKernel(&sensorCtx, &networkCtx);
}

void loop()
{
    kernel->loop();
}