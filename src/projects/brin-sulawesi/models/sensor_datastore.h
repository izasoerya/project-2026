#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>

struct SensorSnapshot
{
    float turbidity;
    float awlr;
    float battery;
};

class SensorDataStore
{
private:
    SensorSnapshot _state;
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

    bool getSnapshot(SensorSnapshot &outSnapshot)
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
