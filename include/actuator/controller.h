#if !defined(CONTROLLER_H)
#define CONTROLLER_H

#include <Arduino.h>

/**
 * @brief Analog Write Controller
 *
 * @param pin output pin
 * @param hz frequency output set
 * @param clipTop set max pwm value
 */
class AnalogController
{
private:
    const uint8_t _pin;
    const uint32_t _hz;
    const uint16_t _clipTop;

public:
    AnalogController(
        const uint8_t pin, const uint32_t hz = 500, const uint16_t clipTop = 128)
        : _pin(pin), _hz(hz), _clipTop(clipTop) {}

    ~AnalogController() {}

    void begin()
    {
        analogWriteFrequency(500);
        pinMode(_pin, OUTPUT);
        analogWrite(_pin, 0);
        digitalWrite(_pin, LOW);
    }

    void control(uint16_t value)
    {
        int val = constrain(value, 0, _clipTop);
        analogWrite(_pin, val);
    }
};

#endif // CONTROLLER_H
