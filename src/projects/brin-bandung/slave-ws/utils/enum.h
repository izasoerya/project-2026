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
    INTERNET_FEAUTRE = 0,
    NTP_FEATURE
};

#endif // ENUM_H
