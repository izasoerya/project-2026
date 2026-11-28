#ifndef BANG_BANG_H
#define BANG_BANG_H

#include <Arduino.h>
#include "actuator/controller.h"

struct BangBangConfig
{
    float upperTemp;
    float bottomTemp;
    float upperHum;
    float bottomHum;
};

struct ActuatorConfig
{
    uint8_t pinExhaustFan;
    uint8_t pinMistMaker;
    uint8_t pinHeater;
};

class BangBangController
{
private:
    BangBangConfig *_configSetPoint;
    AnalogController _fanController;
    AnalogController _mistController;
    ActuatorConfig _configPinout;

public:
    BangBangController(BangBangConfig *setPointConfig,
                       const ActuatorConfig actuatorConfig)
        : _configSetPoint(setPointConfig),
          _configPinout(actuatorConfig),
          _fanController(actuatorConfig.pinExhaustFan, 500, 255),
          _mistController(actuatorConfig.pinMistMaker, 500, 255) {}

    ~BangBangController() {}

    void begin()
    {
        pinMode(_configPinout.pinHeater, OUTPUT);
        analogWrite(_configPinout.pinHeater, 0);
        digitalWrite(_configPinout.pinHeater, LOW);

        _fanController.begin();
        _mistController.begin();
    }

    void control(float temperature, float humidity)
    {
        // Temperature control: FAN
        if (temperature > _configSetPoint->upperTemp)
        {
            _fanController.control(255); // Full ON
        }
        else if (temperature < _configSetPoint->bottomTemp)
        {
            _fanController.control(0); // OFF
        }
        // Humidity control: MIST
        if (humidity > _configSetPoint->upperHum)
        {
            _mistController.control(0); // OFF
        }
        else if (humidity < _configSetPoint->bottomHum)
        {
            _mistController.control(255); // Full ON
        }
    }

    void controlHeater(float temperature, float humidity, bool forcedOff)
    {
        static bool heatDemand = false;

        // Hysteresis for heater
        if (temperature < _configSetPoint->bottomTemp)
            heatDemand = true;
        else if (temperature >= _configSetPoint->upperTemp)
            heatDemand = false;
        if (forcedOff || !heatDemand)
        {
            analogWrite(_configPinout.pinHeater, 0);
            digitalWrite(_configPinout.pinHeater, LOW);
        }
        else
        {
            digitalWrite(_configPinout.pinHeater, HIGH);
            analogWrite(_configPinout.pinHeater, 255);
        }
    }
};

#endif