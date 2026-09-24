#if !defined(SHARED_MODBUS_OBJECT_H)
#define SHARED_MODBUS_OBJECT_H

#include <freertos/semphr.h>
#include <sensor/configs/modbus_sensor.h>
#include "../services/appstate_parser.h"
#include "../models/profile.h"
#include "transmitter/configs/mqtt_module.h"
#include "transmitter/configs/wifi_module.h"

struct FeatureState
{
    bool isMQTTEnabled;
    bool isMQTTAlwaysEnabled;
};

struct SensorContext
{
    Modbustatics &turbidity;
    Modbustatics &awlr;

    SensorContext(Modbustatics &t, Modbustatics &l) : turbidity(t), awlr(l) {}
};

struct NetworkingContext
{
    const char *ssid = GlobalConfig::ssid;
    const char *password = GlobalConfig::password;
    char hostname[32];

    WiFiModule &wifi;

    NetworkingContext(WiFiModule &w) : wifi(w)
    {
        snprintf(hostname, sizeof(hostname), "%s-%d", GlobalConfig::hostname, GlobalConfig::deviceID);
    }
};

struct MQTTContext
{
    static void onMessage(const char *topic, const char *payload) {}
};

struct ApplicationContext
{
    volatile AppState state;
    Modbustatics *mbTurbidity;
    Modbustatics *mbAwlr;
    FeatureState feature;
    Profile *batteryProfile;
    SemaphoreHandle_t mutex;

    ApplicationContext()
    {
        state = AppState::NORMAL;
        mbTurbidity = nullptr;
        mbAwlr = nullptr;
        feature.isMQTTEnabled = true;
        feature.isMQTTAlwaysEnabled = true;
        batteryProfile = nullptr;
        mutex = xSemaphoreCreateMutex();
    }
};

#endif // SHARED_MODBUS_OBJECT_H