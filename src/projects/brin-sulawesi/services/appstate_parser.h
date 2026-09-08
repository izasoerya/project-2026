#if !defined(APP_STATE_PARSER)
#define APP_STATE_PARSER

#include <cstring>
#include "../consts/global_config.h"

enum AppState
{
    NORMAL = 0,
    CALIBRATE_TURBIDITY,
    CALIBRATE_ARR,
    SET_FEATURE,
    APP_STATE_UNKNOWN
};

enum FeaturesEnum
{
    MQTT_RETAIN_ON = 0,
    MQTT_RETAIN_OFF,
    MQTT_ON,
    MQTT_OFF,
    FEATURES_UNKNOWN
};

class CommandHandler
{
private:
public:
    CommandHandler() {}
    ~CommandHandler() {}

    void CommonCommand(uint8_t *data, size_t len)
    {
        const uint8_t MAX_WINDOW = 64;
        static char d[MAX_WINDOW] = {0};
        if (len < MAX_WINDOW - 1)
        {
            memcpy(d, data, len);
            d[len] = '\0';
            while (len > 0 && isspace(d[len - 1]))
                d[--len] = '\0';

            if (strcmp(d, "MYCONFIG") == 0)
            {
                char buffer[128];
                snprintf(buffer, sizeof(buffer),
                         "ID: \n",
                         "SSID: %s\nPasssword: %s\nHostname: %s\n",
                         "MQTT Broker: %s\nUsername: %s\nPassword: %s\n Port: %d",
                         GlobalConfig::deviceID,
                         GlobalConfig::ssid, GlobalConfig::password, GlobalConfig::hostname,
                         GlobalConfig::brokerMqtt, GlobalConfig::usernameMqtt, GlobalConfig::passwordMqtt, GlobalConfig::portMqtt);
                WebSerial.println(buffer);
            }
            else if (strcmp(d, "WIFI_STATUS"))
            {
                char buffer[128];
                snprintf(buffer, sizeof(buffer),
                         "WiFi: %d, RSSI: %d", WiFi.isConnected(), WiFi.RSSI());
                WebSerial.println(buffer);
            }
        }
    }

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

            if (strcmp(d, "APP_NORMAL") == 0)
                return AppState::NORMAL;
            else if (strcmp(d, "APP_CALIBRATE_TURBIDITY") == 0)
                return AppState::CALIBRATE_TURBIDITY;
            else if (strcmp(d, "APP_CALIBRATE_ARR") == 0)
                return AppState::CALIBRATE_ARR;
            else if (strstr(d, "FEAT"))
                return AppState::SET_FEATURE;
        }
        return AppState::APP_STATE_UNKNOWN;
    }

    FeaturesEnum parseFeatureCommand(uint8_t *data, size_t len)
    {
        const uint8_t MAX_WINDOW = 64;
        static char d[MAX_WINDOW] = {0};
        if (len < MAX_WINDOW - 1)
        {
            memcpy(d, data, len);
            d[len] = '\0';
            while (len > 0 && isspace(d[len - 1]))
                d[--len] = '\0';

            if (strcmp(d, "FEAT_MQTT_RETAIN_ON") == 0)
                return FeaturesEnum::MQTT_RETAIN_ON;
            else if (strcmp(d, "FEAT_MQTT_RETAIN_OFF") == 0)
                return FeaturesEnum::MQTT_RETAIN_OFF;
            else if (strcmp(d, "FEAT_MQTT_ON") == 0)
                return FeaturesEnum::MQTT_ON;
            else if (strcmp(d, "FEAT_MQTT_OFF") == 0)
                return FeaturesEnum::MQTT_OFF;
        }
        return FeaturesEnum::FEATURES_UNKNOWN;
    }
};

#endif // APP_STATE_PARSER
