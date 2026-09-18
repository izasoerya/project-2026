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

enum Feature
{
    FEATURE_DISABLED = -1,
    INTERNET_FEAUTRE = 0,
    OTA_FEATURE = 1
};

#endif // ENUM_H
