#ifndef DHT_READING_H
#define DHT_READING_H

#include <Arduino.h>
#include <DHT.h>

class DHTSensor
{
public:
    DHTSensor(const uint8_t pinDHT) : _pinDHT(pinDHT), _dht(pinDHT, DHT22) {}
    ~DHTSensor() {}

    uint8_t begin()
    {
        _dht.begin();
        float temp = _dht.readTemperature();
        if (isnan(temp))
        {
            Serial.println("DHT22 read failed");
            return 0;
        }
        else
        {
            return 1;
        }
    }

    float getTemperature()
    {
        float val = _dht.readTemperature();
        if (isnan(val))
        {
            return 0.0;
        }
        else
        {
            return val;
        }
    }

    float getHumidity()
    {
        float val = _dht.readHumidity();
        if (isnan(val))
        {
            return 0.0;
        }
        else
        {
            return val;
        }
    }

private:
    const uint8_t _pinDHT;
    DHT _dht;
};

#endif