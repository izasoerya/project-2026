#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

#include <ArduinoJson.h>

struct SensorObject
{
    float turbidity;
    float awlr;
    float voltAccu;
    float voltPanel;

    const char *toString()
    {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer),
                 "Turb: %.1f | Lvl: %.1f | VAccu: %.1f | VPanel\n",
                 turbidity, awlr, voltAccu, voltPanel);
        return buffer;
    }

    const char *toJson()
    {
        static JsonDocument doc;
        doc["turbidity"] = turbidity;
        doc["tinggi_muka_air"] = awlr;
        doc["v_accu"] = voltAccu;
        doc["v_panel"] = voltPanel;
        static char buffer[64];
        serializeJson(doc, buffer);
        return buffer;
    }

    const char *toCSV()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%.1f,%.1f,%.1f,%.1f,%d",
                 GlobalConfig::LATITUDE, GlobalConfig::LONGITUDE, turbidity, awlr, voltAccu, voltPanel, 1);
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
                 "TURB: %.1f | LVL: %.1f | VAccu: %.1f | VPanel: %.1f | F_HEAP: %u, | MIN_F_HEAP: %u | LARGEST_BLOCK: %u\n",
                 sensor.turbidity, sensor.awlr, sensor.voltAccu, sensor.voltPanel,
                 ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toString(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TURB: %.1f | LVL: %.1f | VAccu: %.1f | VPanel: %.1f | RSSI: %d | F_HEAP: %lu, | MIN_F_HEAP: %lu | LARGEST_BLOCK: %lu\n",
                 sensor.turbidity, sensor.awlr, sensor.voltAccu, sensor.voltPanel,
                 rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toCSV(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%.1f,%.1f,%.1f,%.1f,%d,%d,%lu,%lu,%lu",
                 GlobalConfig::LATITUDE, GlobalConfig::LONGITUDE, sensor.turbidity, sensor.awlr, sensor.voltAccu, sensor.voltPanel, 1, rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }
};

#endif // SENSOR_DATASTORE_H
