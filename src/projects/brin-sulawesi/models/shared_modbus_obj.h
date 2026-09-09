#if !defined(SHARED_MODBUS_OBJECT_H)
#define SHARED_MODBUS_OBJECT_H

#include <freertos/semphr.h>
#include <sensor/configs/modbus_sensor.h>
#include "../services/appstate_parser.h"
#include "./transmitter/configs/mqtt_module.h"
#include "../models/profile.h"

struct FeatureState
{
    bool isMQTTEnabled;
    bool isMQTTAlwaysEnabled;
};

struct ApplicationContext
{
    volatile AppState state;
    Modbustatics *mbTurbidity;
    Modbustatics *mbAwlr;
    FeatureState feature;
    MQTTModule *mqtt;
    Profile *batteryProfile;
    SemaphoreHandle_t mutex;

    ApplicationContext()
    {
        state = AppState::NORMAL;
        mbTurbidity = nullptr;
        mbAwlr = nullptr;
        feature.isMQTTEnabled = true;
        feature.isMQTTAlwaysEnabled = true;
        mqtt = nullptr;
        batteryProfile = nullptr;
        mutex = xSemaphoreCreateMutex();
    }
};

#endif // SHARED_MODBUS_OBJECT_H