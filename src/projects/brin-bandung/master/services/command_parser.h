#if !defined(COMMAND_PARSER_H)
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "../utils/enum.h"

class CommandParser
{
public:
    static EnabledOTA otaCommand(uint8_t *data, size_t len)
    {
        char buffer[256];
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;
        memcpy(buffer, data, len);
        buffer[len] = '\0';

        if (strcmp(buffer, "ENABLE_OTA_SLAVE_WS") == 0)
        {
            Serial.printf("[INFO] Message Valid: %s\n", buffer);
            return EnabledOTA::SLAVE_WS_ON;
        }
        else if (strcmp(buffer, "DISABLE_OTA_SLAVE_WS") == 0)
        {
            Serial.printf("[INFO] Message Valid: %s\n", buffer);
            return EnabledOTA::SLAVE_WS_OFF;
        }
        else if (strcmp(buffer, "ENABLE_OTA_SLAVE_ARR") == 0)
        {
            Serial.printf("[INFO] Message Valid: %s\n", buffer);
            return EnabledOTA::SLAVE_ARR_ON;
        }
        else if (strcmp(buffer, "DISABLE_OTA_SLAVE_ARR") == 0)
        {
            Serial.printf("[INFO] Message Valid: %s\n", buffer);
            return EnabledOTA::SLAVE_ARR_OFF;
        }
        Serial.printf("[ERROR] Message Invalid: %s\n", buffer);
        return EnabledOTA::INVALID;
    }
};

#endif // COMMAND_PARSER_H
