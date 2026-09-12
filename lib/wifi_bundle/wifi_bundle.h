#ifndef WIFI_BUNDLE_H
#define WIFI_BUNDLE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

#include "transport.h"

class WiFiBundle
{
public:
    WiFiBundle(
        const char *ssid, const char *password, const char *hostname,
        wifi_power_t *txPower = nullptr);
    ~WiFiBundle();

    bool begin(
        const std::function<void()> &onProgress,
        const std::function<void()> &onTimeout);
    const char *localIP();
    int8_t getdBm();
    bool reconnect(bool blocking, std::function<void(bool)> *onResult = nullptr);
    bool disconnect();

    void setTxPower(wifi_power_t config) { WiFi.setTxPower(config); }
    void setTransport(DataTransport *transport) { _transport = transport; }
    DataTransport *getTransport() const { return _transport; }

    int post(const char *url, const char *payload);

private:
    const char *_ssid;
    const char *_password;
    const char *_hostname;
    char _localIP[16];

    DataTransport *_transport = nullptr;
    wifi_power_t *_txPowerConfig = nullptr;

    uint8_t _counterReset = 0;
    const uint8_t _maxRetry = 3;

    const std::function<void()> *onProgress = nullptr;
    const std::function<void()> *onTimeout = nullptr;

    bool _setupMDNS()
    {
        if (!MDNS.begin(_hostname))
            return false;
        MDNS.addService("http", "tcp", 80);
        return true;
    }
};

#endif