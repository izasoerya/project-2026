#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>
#include <ArduinoJson.h>

struct SensorObject
{
    float turbidity;
    float awlr;
    float battery;

    const char *toString()
    {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer),
                 "TURB: %.1f | LVL: %.1f | BATT: %.1f\n",
                 turbidity, awlr, battery);
        return buffer;
    }

    const char *toJson()
    {
        static JsonDocument doc;
        doc["turbidity"] = turbidity;
        doc["level"] = awlr;
        doc["battery"] = battery;
        static char buffer[64];
        serializeJson(doc, buffer);
        return buffer;
    }
};

#endif // SENSOR_DATASTORE_H
