#if !defined(ACTAUTOR_DRIVER_H)
#define ACTAUTOR_DRIVER_H

#include <Arduino.h>
#include "projects/zf-projects/zf-node-heater/consts/global_definition.h"

class ActuatorDriver
{
private:
    bool state = false;

public:
    ActuatorDriver()
    {
        pinMode(GlobalConfig::PIN_HEATER_RELAY, OUTPUT);
        pinMode(GlobalConfig::PIN_GREEN_LED, OUTPUT);
        digitalWrite(GlobalConfig::PIN_HEATER_RELAY, LOW);
        digitalWrite(GlobalConfig::PIN_GREEN_LED, LOW);
    }

    void controlActuator(bool on)
    {
        digitalWrite(GlobalConfig::PIN_HEATER_RELAY, on);
        digitalWrite(GlobalConfig::PIN_GREEN_LED, on);
    }

    bool getActuatorState()
    {
        return digitalRead(GlobalConfig::PIN_HEATER_RELAY);
    }
};

#endif // ACTAUTOR_DRIVER_H
