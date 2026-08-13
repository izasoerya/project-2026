#ifndef HX711_READING_H
#define HX711_READING_H

#include <Arduino.h>
#include "HX711.h"
#include "../base_sensor.h"

/**
 * @brief HX711 Sensor
 *
 * @param pinDT pin dt of the sensor
 * @param pinCLK pin clk of the sensor
 */
class HX711Sensor : public BaseSensor
{
private:
    const uint8_t _pinDT;
    const uint8_t _pinCLK;
    HX711 _sc;

    std::function<float(float)> _interceptor;

public:
    HX711Sensor(
        unsigned char id, const char *name,
        const uint8_t pinDT, const uint8_t pinCLK,
        std::function<float(float)> interceptor = nullptr)
        : BaseSensor(id, name),
          _pinDT(pinDT), _pinCLK(pinCLK),
          _interceptor(interceptor) {}

    ~HX711Sensor() override = default;

    bool begin()
    {
        _sc.begin(_pinDT, _pinCLK);
        _sc.set_scale(0);
        _sc.set_offset(0);
        return true;
    }

    float read() override
    {
        return _interceptor(_sc.read_average(15));
    }
};

#endif