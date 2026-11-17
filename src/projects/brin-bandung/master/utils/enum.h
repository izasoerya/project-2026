#if !defined(ENUM_H)
#define ENUM_H

enum FeatureStatus
{
    NTP = 0,
    INTERNET,
    MQTT,
    WIREGUARD,
    MODBUS,
    WORKING
};

enum EnabledOTA
{
    INVALID = 0,
    SLAVE_ARR_ON,
    SLAVE_ARR_OFF,
    SLAVE_WS_ON,
    SLAVE_WS_OFF
};

enum DeviceType
{
    MASTER,
    SLAVE_WS,
    SLAVE_ARR
};

enum CommandType
{
    SET_OTA,
    SET_DELAY,
    RESTART_DEVICE
};

#endif // ENUM_H
