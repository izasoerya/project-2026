#if !defined(MODELS_H)
#define MODELS_H

#include <Arduino.h>
#include "../utils/parser.h"

struct AquaponicSensorEntity
{
    uint16_t lightIntensity;
    float waterLevel;
    float waterPH;
    float waterTemperature;
    float waterTDS;

    const char *toString()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "Level: %.1f\nPH: %.2f\nTemperature: %.1f\nTDS: %.1f\nLux: %d",
                 waterLevel, waterPH, waterTemperature, waterTDS, lightIntensity);
        return buffer;
    }
};

struct AquaponicSystemEntity
{
    uint32_t freeHeap;
    uint32_t largestFreeBlock;
    uint32_t minFreeHeap;
    esp_reset_reason_t lastResetReason;

    const char *toString()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "freeHeap: %lu\nfreeBlock: %lu\nminFreeHeap: %lu\nResetReason: %s",
                 freeHeap, largestFreeBlock, minFreeHeap, Parser::parseResetReasonESP(lastResetReason));
        return buffer;
    }
};

#endif // MODELS_H
