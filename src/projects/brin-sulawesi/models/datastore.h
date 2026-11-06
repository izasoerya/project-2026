#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

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

struct SensorLogAggregate
{
    SensorObject sensor;

    SensorLogAggregate(SensorObject s) : sensor(s) {}

    const char *toString()
    {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer),
                 "TURB: %.1f | LVL: %.1f | BATT: %.1f | F_HEAP: %u, | MIN_F_HEAP: %u | LARGEST_BLOCK: %u\n",
                 sensor.turbidity, sensor.awlr, sensor.battery,
                 ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toString(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TURB: %.1f | LVL: %.1f | BATT: %.1f | RSSI: %d | F_HEAP: %lu, | MIN_F_HEAP: %lu | LARGEST_BLOCK: %lu\n",
                 sensor.turbidity, sensor.awlr, sensor.battery,
                 rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toCSV(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%.1f,%.1f,%.1f,%d,%d,%lu,%lu,%lu",
                 -7.8367746, 110.3421102, sensor.turbidity, sensor.awlr, sensor.battery, 1, rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }
};

#endif // SENSOR_DATASTORE_H
