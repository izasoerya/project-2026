#if !defined(MODELS_H)
#define MODELS_H

#include <Arduino.h>
#include "../utils/parser.h"

struct PikohidroSensorEntity
{
    float waterTurbidity;
    float waterPH;
    float waterTDS;
    float powerIn;
    float powerOut;

    const char *toString()
    {
        static char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "TURB: %.1f | PH: %.2f | TDS: %.1f | P_IN: %.1f | P_OUT: %.1f\n",
                 waterTurbidity, waterPH, waterTDS, powerIn, powerOut);
        return buffer;
    }
};

struct PikohidroSystemEntity
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
