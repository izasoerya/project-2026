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
    esp_sleep_enable_timer_wakeup(GlobalConfig::DELAY_SAMPLING * 1000ULL); // Configure sleep for 10s

    Profile batteryProfileDefault(
        GlobalConfig::BATTERY_DANGER, GlobalConfig::BATTERY_WARNING);

    Serial.begin(115200);
    GlobalConfig::mbSerial.begin(9600, SERIAL_8N1, 20, 21);
    static Modbustatics *arrSensor = nullptr;
    static INA226SensorInterface *inaAccuSensor = nullptr;
    static INA226SensorInterface *inaSolarSensor = nullptr;
    for (uint8_t i = 0; i < GlobalConfig::DEVICE_COUNT; i++)
    {
        if (arr_list[i].getId() == DEVICE_ID)
            arrSensor = &arr_list[i];
        if (ina_accu_list[i].getId() == DEVICE_ID) // INA id start from 1
            inaAccuSensor = &ina_accu_list[i];
        if (ina_solar_list[i].getId() == DEVICE_ID) // INA id start from 1
            inaSolarSensor = &ina_solar_list[i];
    }
    static SensorContext sensorCtx(*arrSensor, *inaAccuSensor, *inaSolarSensor);

    static WiFiModule inet(
        GlobalConfig::ssid, GlobalConfig::password, GlobalConfig::hostname,
        wifi_power_t::WIFI_POWER_8_5dBm);
    static MQTTWebsocketSecure mqtt(GlobalConfig::BROKER_MQTT, GlobalConfig::PORT_MQTT);
    static NetworkingContext networkCtx(inet, mqtt);

    FSMKernel kernel = FSMKernel(&sensorCtx, &networkCtx);
    while (1)
        kernel.loop();
}

void loop() {}