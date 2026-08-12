#if !defined(WIFI_MODULE_H)
#define WIFI_MODULE_H

#include <WiFi.h>
#include <HTTPClient.h>

#include "wifi_bundle.h"
#include "../base_transmitter.h"

/**
 * @brief WiFi Module Class
 *
 * @param ssid const char* (WiFi SSID)
 * @param password const char* (WiFi Password)
 * @param hostname const char* (Hostname for the Device)
 *
 */
class WiFiModule : public BaseTransmitter
{
private:
    WiFiBundle _inet;
    BasicHTTPTransport *basicConfig = nullptr;
    SupabaseTransport *supabaseConfig = nullptr;
    wifi_power_t _txCConfig;

public:
    WiFiModule(
        const char *ssid, const char *password, const char *hostname,
        wifi_power_t txConfig = WIFI_POWER_19_5dBm)
        : _txCConfig(txConfig), _inet(WiFiBundle(ssid, password, hostname, &_txCConfig)) {}
    ~WiFiModule() override = default;

    const char *localIP()
    {
        return _inet.localIP();
    }

    void setTransport(BasicHTTPTransport *basicTransport)
    {
        basicConfig = basicTransport;
    }

    void setTransport(SupabaseTransport *supabaseTransport)
    {
        supabaseConfig = supabaseTransport;
    }

    bool begin(const std::function<void()> &onProgress,
               const std::function<void()> &onTimeout)
    {
        if (_inet.begin(onProgress, onTimeout)) // Restart on fail set to true
            return true;
        return false;
    }

    IPAddress resolveMDNS(const char *hostname)
    {
        IPAddress query;
        uint8_t attempts = 0;
        while (attempts < 10)
        {
            query = MDNS.queryHost(hostname);
            if (query != IPADDR_NONE && query.toString() != "0.0.0.0")
                return query;
            attempts++;
            delay(1000);
        }
        return IPAddress(0, 0, 0, 0); // Explicit failure
    }

    void reconnect()
    {
        _inet.reconnect();
    }

    int8_t getRssi()
    {
        return _inet.getdBm();
    }

    int16_t send(const char *url, const char *payload)
    {
        if (basicConfig != nullptr)
            _inet.setTransport(basicConfig);
        else if (supabaseConfig != nullptr)
            _inet.setTransport(supabaseConfig);
        return _inet.post(url, payload);
    }
};

#endif // WIFI_MODULE_H
