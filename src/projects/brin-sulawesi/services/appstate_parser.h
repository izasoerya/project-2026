#if !defined(APP_STATE_PARSER)
#define APP_STATE_PARSER

#include <cstring>

enum AppState
{
    NORMAL = 0,
    CALIBRATE_TURBIDITY,
    CALIBRATE_ARR,
    UNKNOWN
};

class CommandHandler
{
private:
public:
    CommandHandler() {}
    ~CommandHandler() {}

    AppState parseCommand(uint8_t *data, size_t len)
    {
        const uint8_t MAX_WINDOW = 64;
        static char d[MAX_WINDOW] = {0};
        if (len < MAX_WINDOW - 1)
        {
            memcpy(d, data, len);
            d[len] = '\0';
            while (len > 0 && isspace(d[len - 1]))
                d[--len] = '\0';

            if (strcmp(d, "NORMAL") == 0)
                return AppState::NORMAL;
            else if (strcmp(d, "CALIBRATE_TURBIDITY") == 0)
                return AppState::CALIBRATE_TURBIDITY;
            else if (strcmp(d, "CALIBRATE_ARR") == 0)
                return AppState::CALIBRATE_ARR;
        }
        return AppState::UNKNOWN;
    }
};

#endif // APP_STATE_PARSER
