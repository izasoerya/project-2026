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
    }
};

class SensorDataStore
{
private:
    SensorObject _state;
    SemaphoreHandle_t _mutex;

public:
    SensorDataStore() : _state{0.0f, 0.0f, 0.0f}
    {
        _mutex = xSemaphoreCreateMutex();
    }

    ~SensorDataStore()
    {
        if (_mutex != nullptr)
            vSemaphoreDelete(_mutex);
    }

    bool update(float turbidity, float temperature)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            _state.turbidity = turbidity;
            xSemaphoreGive(_mutex);
            return true;
        }
        return false; // Timeout
    }

    bool getSnapshot(SensorObject &outSnapshot)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            outSnapshot = _state; // Struct copy
            xSemaphoreGive(_mutex);
            return true;
        }
        return false;
    }
};

// Global or module-scoped singleton instance
extern SensorDataStore globalSensorStore;

#endif // SENSOR_DATASTORE_H
