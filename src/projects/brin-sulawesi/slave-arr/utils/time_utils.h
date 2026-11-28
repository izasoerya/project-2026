#if !defined(TIME_UTILS_H)
#define TIME_UTILS_H

#include <Arduino.h>
#include <esp_timer.h>

class TimeUtils
{
public:
    // Milliseconds since boot, 64-bit so it never wraps (millis() wraps after ~49 days)
    static uint64_t uptimeMs()
    {
        return static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
    }
};

#endif // TIME_UTILS_H
