#if !defined(COMMAND_PARSER_H)
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "../utils/enum.h"

struct Command
{
    DeviceType device;
    CommandType cmd;
    uint32_t payload;
    size_t len;
};

class CommandParser
{
public:
    static Command parse(const char *buffer)
    {
        Command cmd = {};

        if (strncmp(buffer, "MASTER:", 7) == 0)
        {
            cmd.device = MASTER;
            buffer += 7;
        }
        else if (strncmp(buffer, "SLAVE_WS:", 9) == 0)
        {
            cmd.device = SLAVE_WS;
            buffer += 9;
        }
        else if (strncmp(buffer, "SLAVE_ARR:", 10) == 0)
        {
            cmd.device = SLAVE_ARR;
            buffer += 10;
        }

        if (strncmp(buffer, "SET_OTA=", 8) == 0)
        {
            cmd.cmd = SET_OTA;
            cmd.payload = strtoul(buffer + 8, nullptr, 10);
        }
        else if (strncmp(buffer, "SET_DELAY=", 10) == 0)
        {
            cmd.cmd = SET_DELAY;
            cmd.payload = strtoul(buffer + 10, nullptr, 10);
        }
        else if (strncmp(buffer, "RESTART_DEVICE", 14) == 0)
        {
            cmd.cmd = RESTART_DEVICE;
        }

        return cmd;
    }

    static Command parseIncoming(uint8_t *data, size_t len)
    {
        static char buffer[256];
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;
        memcpy(buffer, data, len);
        buffer[len] = '\0';
        Command resultParse = CommandParser::parse(buffer);
        return resultParse;
    }
};

#endif // COMMAND_PARSER_H
