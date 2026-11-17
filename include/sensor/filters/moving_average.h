#if !defined(MOVING_AVERAGE)
#define MOVING_AVERAGE

#include "base_filter.h"

/**
 * @brief Moving Average Filter (Max window: 50)
 *
 * @param uint8_t window_size
 */
class MovingAverageFilter : public BaseFilter
{
private:
    static const uint8_t MAX_WINDOW = 50;
    float _buffer[MAX_WINDOW];
    uint8_t _windowSize;
    uint8_t _index = 0;

public:
    MovingAverageFilter(uint8_t windowSize = 10)
        : _windowSize(windowSize)
    {
        if (_windowSize > MAX_WINDOW)
            _windowSize = MAX_WINDOW;

        for (uint8_t i = 0; i < _windowSize; i++)
            _buffer[i] = 0;
    }
    ~MovingAverageFilter() override = default;

    float filter(float &raw) override
    {
        _buffer[_index] = raw;
        _index = (_index + 1) % _windowSize;

        float sum = 0.0f;
        for (uint8_t i = 0; i < _windowSize; i++)
            sum += _buffer[i];
        raw = sum / _windowSize;
        return raw;
    }
};

/**
 * @brief Moving average filter with trimming the biggest and smallest number
 * based on trim count (Max window: 50)
 *
 * @param uint8_t window_size
 * @param uint8_t trim_size
 */
class TrimmedMovingAverage : public BaseFilter
{
private:
    static const uint8_t MAX_WINDOW = 50;
    float _buffer[MAX_WINDOW];
    uint8_t _windowSize;
    uint8_t _trim;
    uint8_t _index = 0;

    float _value = 0;

public:
    TrimmedMovingAverage(uint8_t windowSize = 20, uint8_t trim = 5)
        : _windowSize(windowSize), _trim(trim)
    {
        if (_windowSize > MAX_WINDOW)
            _windowSize = MAX_WINDOW;

        for (uint8_t i = 0; i < _windowSize; i++)
            _buffer[i] = 0;
    }
    ~TrimmedMovingAverage() override = default;

    float filter(float &raw) override
    {
        _buffer[_index] = raw;
        _index = (_index + 1) % _windowSize;

        float _sortBuffer[_windowSize]; // May cause stack overflow if too big
        memcpy(_sortBuffer, _buffer, _windowSize * sizeof(float));
        std::sort(_sortBuffer, _sortBuffer + _windowSize);

        double sum = 0;
        for (int i = _trim; i < _windowSize - _trim; i++)
            sum += _sortBuffer[i];
        raw = sum / (_windowSize - 2 * _trim);
        _value = raw;
        return _value;
    }

    float getValue() { return _value; }
};

#endif // MOVING_AVERAGE
