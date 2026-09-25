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

enum ApplicationState
{
    UNKNOWN_STATE = 0,
    SAMPLING_STATE,
    CONNECT_STATE,
    PUBLISH_STATE,
};

enum FeaturesEnum
{
    MQTT_RETAIN_ON = 0,
    MQTT_RETAIN_OFF,
    MQTT_ON,
    MQTT_OFF,
    FEATURES_UNKNOWN
};

#endif // APP_STATE_PARSER
