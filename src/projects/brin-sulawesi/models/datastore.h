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
        doc["tinggi_muka_air"] = awlr;
        doc["battery"] = battery;
        static char buffer[64];
        serializeJson(doc, buffer);
        return buffer;
    }

    const char *toCSV()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%.1f,%.1f,%.1f,%d",
                 -7.8367746, 110.3421102, turbidity, awlr, battery, 1);
        return buffer;
    }
};

#endif // SENSOR_DATASTORE_H
