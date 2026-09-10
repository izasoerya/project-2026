#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#include <Arduino.h>
#include "../utils/utils.h"

struct SensorObject
{
    float temperature;
    float humidity;
    float windSpeed;
    WindDirectionEnum windDirection;
};

class SensorDatastore
{
private:
    SensorObject _sensor;
    SemaphoreHandle_t _mutex;

public:
    SensorDatastore()
    {
        _mutex = xSemaphoreCreateMutex();
    }
    ~SensorDatastore()
    {
        if (_mutex != nullptr)
            vSemaphoreDelete(_mutex);
    }

    bool update(SensorObject sensor)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            _sensor.temperature = sensor.temperature;
            _sensor.humidity = sensor.humidity;
            _sensor.windSpeed = sensor.windSpeed;
            _sensor.windDirection = sensor.windDirection;

            xSemaphoreGive(_mutex);
            return true;
        }
        return false; // Timeout
    }

    bool getSnapshot(SensorObject &outSnapshot)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            outSnapshot = _sensor;
            xSemaphoreGive(_mutex);
            return true;
        }
        return false;
    }
};

extern SensorDatastore singletonSensor;

#endif // SENSOR_DATASTORE_H
