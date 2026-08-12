#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct SensorProp
{
    float value = 0.0;
    int8_t status;
};

struct SensorData
{
    byte id;
    SensorProp temperature_soil;
    SensorProp humidity_soil;
    SensorProp ph;
    SensorProp weight_breed;
    SensorProp weight_yield;
    SensorProp temperature_air;
    SensorProp humidity_air;
    unsigned long created_at;

    JsonDocument toJsonDocument() const
    {
        JsonDocument doc;
        doc["device_id"] = id;
        doc["created_at"] = created_at;
        doc["temperature_soil"] = temperature_soil.value;
        doc["humidity_soil"] = humidity_soil.value;
        doc["ph"] = ph.value;
        doc["weight_breed"] = weight_breed.value;
        doc["weight_yield"] = weight_yield.value;
#if defined(SIBOB_1)
        doc["weight"] = (((weight_breed.value * 0.35 + weight_yield.value * 0.85) - 83940.7) * -0.01011249) - 1810.866409;
#elif defined(SIBOB_2)
        doc["weight"] = (((weight_breed.value * 0.35 + weight_yield.value * 0.85) - 314530.7) * 0.01023748) - 88.3;
#endif
        doc["temperature_air"] = temperature_air.value;
        doc["humidity_air"] = humidity_air.value;

        String status = "";
        if (temperature_soil.status != 0)
            status += "T";
        if (humidity_soil.status != 0)
            status += "H";
        if (ph.status != 0)
            status += "P";
        if (weight_breed.status != 0)
            status += "W";
        if (weight_yield.status != 0)
            status += "Y";
        if (temperature_air.status != 0)
            status += "A";
        if (humidity_air.status != 0)
            status += "U";
        doc["status"] = status;
        return doc;
    }
};

struct ActuatorData
{
    uint16_t exhaust_fan;
    uint16_t mist_maker;
    uint16_t heater;
    unsigned long created_at;

    JsonDocument toJsonDocument() const
    {
        JsonDocument doc;
        doc["exhaust_fan"] = exhaust_fan;
        doc["mist_maker"] = mist_maker;
        doc["heater"] = heater;
        return doc;
    }
};

#endif // MODELS_H