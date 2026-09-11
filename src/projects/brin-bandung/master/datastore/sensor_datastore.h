#if !defined(SENSOR_DATASTORE_H)
#define SENSOR_DATASTORE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../utils/utils.h"
#include "../utils/parser.h"

struct SensorRainfallObject
{
    float rainfall;
};

struct SensorWSObject
{
    float temperature;
    float humidity;
    float windSpeed;
    WindDirectionEnum windDirection;
};

struct SensorPublishableObject
{
    float temperature;
    float humidity;
    float windSpeed;
    WindDirectionEnum windDirection;
    float rainfall;

    size_t toJson(char *out, size_t outSize) const
    {
        JsonDocument doc;
        doc["device_id"] = DEVICE_ID;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["wind_speed"] = windSpeed;
        doc["wind_direction"] = Parser::parseWindDirection(windDirection);
        doc["rainfall"] = rainfall;

        return serializeJson(doc, out, outSize);
    }
};

class SensorRainfallDatastore
{
private:
    SensorRainfallObject _sensor;
    SemaphoreHandle_t _mutex;

public:
    SensorRainfallDatastore()
    {
        _mutex = xSemaphoreCreateMutex();
    }
    ~SensorRainfallDatastore()
    {
        if (_mutex != nullptr)
            vSemaphoreDelete(_mutex);
    }

    bool update(SensorRainfallObject sensor)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            _sensor.rainfall = sensor.rainfall;

            xSemaphoreGive(_mutex);
            return true;
        }
        return false; // Timeout
    }

    bool getSnapshot(SensorRainfallObject &outSnapshot)
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

class SensorWSDatastore
{
private:
    SensorWSObject _sensor;
    SemaphoreHandle_t _mutex;

public:
    SensorWSDatastore()
    {
        _mutex = xSemaphoreCreateMutex();
    }
    ~SensorWSDatastore()
    {
        if (_mutex != nullptr)
            vSemaphoreDelete(_mutex);
    }

    bool update(SensorWSObject sensor)
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

    bool getSnapshot(SensorWSObject &outSnapshot)
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

class SensorPublishableDatastore
{
private:
    SensorPublishableObject _sensor;
    SemaphoreHandle_t _mutex;

public:
    SensorPublishableDatastore()
    {
        _mutex = xSemaphoreCreateMutex();
    }
    ~SensorPublishableDatastore()
    {
        if (_mutex != nullptr)
            vSemaphoreDelete(_mutex);
    }

    bool update(SensorPublishableObject sensor)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            _sensor.temperature = sensor.temperature;
            _sensor.humidity = sensor.humidity;
            _sensor.windSpeed = sensor.windSpeed;
            _sensor.windDirection = sensor.windDirection;
            _sensor.rainfall = sensor.rainfall;

            xSemaphoreGive(_mutex);
            return true;
        }
        return false; // Timeout
    }

    bool getSnapshot(SensorPublishableObject &sensor)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            sensor = _sensor;
            xSemaphoreGive(_mutex);
            return true;
        }
        return false;
    }
};

extern SensorRainfallDatastore singletonSensorRainfall;
extern SensorWSDatastore singletonSensorWS;
extern SensorPublishableDatastore singletonSensorFull;

#endif // SENSOR_DATASTORE_H
