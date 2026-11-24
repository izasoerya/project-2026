#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

#include <ArduinoJson.h>

struct SensorObject
{
    float tipping;
    float voltAccu;
    float voltPanel;

    const char *toString()
    {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer),
                 "Tip: %.1f | VAccu: %.1f | VPanel: %.1f\n",
                 tipping, voltAccu, voltPanel);
        return buffer;
    }

    const char *toJson()
    {
        static JsonDocument doc;
        doc["tipping"] = tipping;
        doc["v_accu"] = voltAccu;
        doc["v_solar_panel"] = voltPanel;
        static char buffer[64];
        serializeJson(doc, buffer);
        return buffer;
    }

    const char *toCSV()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%d,%.1f,%.1f,%d",
                 GlobalConfig::latitude, GlobalConfig::longitude,
                 tipping, voltAccu, voltPanel, 1);
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
                 "Tip: %.1f | VAccu: %.1f | VPanel: %.1f | F_HEAP: %lu, | MIN_F_HEAP: %lu | LARGEST_BLOCK: %lu\n",
                 sensor.tipping, sensor.voltAccu, sensor.voltPanel,
                 ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toString(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "Tip: %.1f | VAccu: %.1f | VPanel: %.1f | RSSI: %d | F_HEAP: %lu, | MIN_F_HEAP: %lu | LARGEST_BLOCK: %lu\n",
                 sensor.tipping, sensor.voltAccu, sensor.voltPanel,
                 rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }

    const char *toCSV(int8_t rssi)
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "%.6f,%.6f,%.1f,%.1f,%.1f,%d,%d,%lu,%lu,%lu",
                 GlobalConfig::latitude, GlobalConfig::longitude,
                 sensor.tipping, sensor.voltAccu, sensor.voltPanel, 1,
                 rssi, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
        return buffer;
    }
};

#endif // SENSOR_DATASTORE_H
