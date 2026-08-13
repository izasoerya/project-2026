#ifndef BANG_BANG_H
#define BANG_BANG_H

#include <Arduino.h>

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
    BangBangConfig _configSetPoint;
    ActuatorConfig _configActuator;

public:
    // Lazy separate body file sorry
    BangBangController(const BangBangConfig setPointConfig,
                       const ActuatorConfig actuatorConfig)
    {
        _configSetPoint = setPointConfig;
        _configActuator = actuatorConfig;
    }
    ~BangBangController() {}

    void control(float temperature, float humidity) const
    {
        if (temperature > _configSetPoint.upperTemp)
        {
            // Turn off exhaust fan
            digitalWrite(_configActuator.pinExhaustFan, HIGH);
            Serial.print("FAN IS ON  pin: ");
            Serial.println(_configActuator.pinExhaustFan);
        }
        else if (temperature < _configSetPoint.bottomTemp)
        {
            // Turn on exhaust fan
            digitalWrite(_configActuator.pinExhaustFan, LOW);
            Serial.print("FAN IS OFF pin: ");
            Serial.println(_configActuator.pinExhaustFan);
        }

        if (humidity > _configSetPoint.upperHum)
        {
            // Turn off mist maker
            digitalWrite(_configActuator.pinMistMaker, LOW);
            Serial.print("MIST IS OFF  pin: ");
            Serial.println(_configActuator.pinMistMaker);

            // Turn off exhaust fan
            digitalWrite(_configActuator.pinExhaustFan, HIGH);
            Serial.print("FAN IS ON  pin: ");
            Serial.println(_configActuator.pinExhaustFan);
        }
        else if (humidity < _configSetPoint.bottomHum)
        {
            // Turn on mist maker
            digitalWrite(_configActuator.pinMistMaker, HIGH);
            Serial.print("MIST IS ON  pin: ");
            Serial.println(_configActuator.pinMistMaker);

            // Turn on exhaust fan
            digitalWrite(_configActuator.pinExhaustFan, LOW);
            Serial.print("FAN IS OFF pin: ");
            Serial.println(_configActuator.pinExhaustFan);
        }
    }

    void controlHeater(float temperature, float humidity, bool forcedOff) const
    {
        static bool heatDemand = false;
        if (temperature < _configSetPoint.bottomTemp)
        {
            heatDemand = true;
        }
        else if (temperature >= _configSetPoint.upperTemp)
        {
            heatDemand = false;
        }

        if (forcedOff || !heatDemand)
        {
            analogWrite(_configActuator.pinHeater, 0);
            Serial.print("HEATER IS OFF  pin: ");
            Serial.println(_configActuator.pinHeater);
        }
        else
        {
            analogWrite(_configActuator.pinHeater, 10);
            Serial.print("HEATER IS ON  pin: ");
            Serial.println(_configActuator.pinHeater);
        }
    }
};

#endif