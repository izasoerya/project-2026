#if !defined(UTILS_H)
#define UTILS_H

enum AppState
{
    NORMAL_MODE = 0,
    ENABLE_LOGGING_ADS
};

enum WindDirectionEnum
{
    NORTH = 0,
    NORTH_EAST = 1,
    EAST = 2,
    SOUTHEAST = 3,
    SOUTH = 4,
    SOUTHWEST = 5,
    WEST = 6,
    NORTHWEST = 7
};

class Utils
{
public:
    static uint16_t toDeciU16(float value)
    {
        float scaled = value * 10.0F;
        if (scaled < 0.0F)
            return 0;
        if (scaled > 65535.0F)
            return 65535;
        return static_cast<uint16_t>(roundf(scaled));
    }
};

#endif // UTILS_H
