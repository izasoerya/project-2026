#if !defined(WIFI_BLYNK)
#define WIFI_BLYNK

#include <BlynkSimpleEsp32.h>
#include "../lib/wifi_bundle/wifi_bundle.h"

class WiFiBlynk;

inline WiFiBlynk *&wifiBlynkInstance()
{
    static WiFiBlynk *instance = nullptr;
    return instance;
}

class WiFiBlynk
{
private:
    const char *_authToken;
    const char *_hostname;

    std::function<void(int, bool)> _callback;
    WiFiBundle _wifi;

    bool _setupMDNS()
    {
        if (!MDNS.begin(_hostname))
            return false;
        MDNS.addService("http", "tcp", 80);
        return true;
    }

public:
    WiFiBlynk(
        const char *authToken,
        const char *ssid, const char *password, const char *hostname,
        wifi_power_t txPower = wifi_power_t::WIFI_POWER_19_5dBm,
        std::function<void(int, bool)> callback = nullptr)
        : _authToken(authToken),
          _hostname(hostname), _wifi(WiFiBundle(ssid, password, hostname, &txPower)),
          _callback(callback)
    {
        wifiBlynkInstance() = this;
    }
    ~WiFiBlynk() {}

    bool begin()
    {
        _wifi.begin(
            []()
            { Serial.print("."); },
            []()
            { esp_restart(); });
        Blynk.config(_authToken);
        Blynk.connect();
        return true;
    }

    int8_t getSignalStrength()
    {
        return _wifi.getdBm();
    }

    void send(uint8_t virtualPin, float value)
    {
        Blynk.virtualWrite(virtualPin, value);
    }

    void run()
    {
        Blynk.run();
    }

    void onCallback(uint8_t pin, bool value)
    {
        _callback(pin, value);
    }

    void reconnect()
    {
        _wifi.reconnect(true, nullptr);
    }
};

BLYNK_WRITE_DEFAULT()
{
    if (wifiBlynkInstance() != nullptr)
    {
        wifiBlynkInstance()->onCallback(request.pin, param.asInt());
    }
}

#endif // WIFI_BLYNK
