#if !defined(WIFI_MODULE_H)
#define WIFI_MODULE_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

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

    const char *_ssid;
    const char *_password;
    const char *_hostname;

    const std::function<void()> *_onProgress;
    const std::function<void()> *_onTimeout;

public:
    WiFiModule(
        const char *ssid, const char *password, const char *hostname,
        wifi_power_t txConfig = WIFI_POWER_19_5dBm)
        : _txCConfig(txConfig), _ssid(ssid), _password(password), _hostname(hostname),
          _inet(WiFiBundle(ssid, password, hostname, &_txCConfig)) {}
    ~WiFiModule() override = default;

    const char *localIP()
    {
        return _inet.localIP();
    }

    void setTransport(BasicHTTPTransport *basicTransport) { basicConfig = basicTransport; }
    void setTransport(SupabaseTransport *supabaseTransport) { supabaseConfig = supabaseTransport; }

    bool begin(const std::function<void()> &onProgress,
               const std::function<void()> &onTimeout)
    {
        _onProgress = &onProgress;
        _onTimeout = &onTimeout;
        if (_inet.begin(onProgress, onTimeout)) // Restart on fail set to true
            return true;
        return false;
    }

    void beginNB(std::function<void(bool)> onResult = nullptr)
    {
        struct AsyncWiFiTask
        {
            const char *ssid;
            const char *password;
            const char *hostname;
            wifi_power_t txConfig;
            std::function<void(bool)> callback;
        };

        AsyncWiFiTask *params = new AsyncWiFiTask{
            _ssid, _password, _hostname, _txCConfig, onResult};

        xTaskCreate([](void *pvParam)
                    {
                    AsyncWiFiTask *p = static_cast<AsyncWiFiTask *>(pvParam);
                    
                    WiFi.setHostname(p->hostname);
                    WiFi.begin(p->ssid, p->password);
                    WiFi.setTxPower(p->txConfig);

                    Serial.printf("Hostname: %s\n", p->hostname);
                    
                    uint32_t start = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000)
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                    
                    bool success = WiFi.status() == WL_CONNECTED;
                    if (p->callback)    
                        p->callback(success);
                    
                    if (success) {
                        if (!MDNS.begin(p->hostname))
                        MDNS.addService("http", "tcp", 80);
                    }
                    
                    delete p;
                    vTaskDelete(NULL); },
                    "Connect WiFi", 2048, params, 1, nullptr);
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

    bool reconnect(bool blocking, std::function<void(bool)> *onResult = nullptr)
    {
        bool connected = _inet.reconnect(blocking, onResult);
        return connected;
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
