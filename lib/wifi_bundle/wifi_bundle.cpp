#include "wifi_bundle.h"

WiFiBundle::WiFiBundle(
    const char *ssid, const char *password, const char *hostname,
    wifi_power_t *txPower)
    : _ssid(ssid), _password(password), _hostname(hostname),
      _txPowerConfig(txPower) {}

WiFiBundle::~WiFiBundle()
{
    delete _transport;
}

bool WiFiBundle::begin(
    const std::function<void()> &onProgress,
    const std::function<void()> &onTimeout)
{
    this->onProgress = &onProgress;
    this->onTimeout = &onTimeout;

    WiFi.mode(WIFI_STA);
    WiFi.hostname(_hostname);
    WiFi.begin(_ssid, _password);

    if (_txPowerConfig != nullptr)
        WiFi.setTxPower(*_txPowerConfig);

    uint8_t counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        counter++;
        onProgress();
        if (counter > 40) // 20 second
            onTimeout();
    }
    if (!_setupMDNS())
        onTimeout();

    return true;
}

const char *WiFiBundle::localIP()
{
    IPAddress ip = WiFi.localIP();
    snprintf(_localIP, sizeof(_localIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return _localIP;
}

int8_t WiFiBundle::getdBm()
{
    return WiFi.RSSI();
}

bool WiFiBundle::reconnect(
    bool blocking = true,
    std::function<void(bool)> *onResult)
{
    const uint32_t currentTime = millis();

    if (blocking)
    {
        while (millis() < currentTime + 5000)
            if (WiFi.status() == WL_CONNECTED)
                return true;

        if (WiFi.disconnect())
        {
            bool connected = begin(*onProgress, *onTimeout);
            return connected;
        }
        return false;
    }
    else
    {
        struct ReconnectParam
        {
            WiFiBundle *bundle;
            std::function<void(bool)> callback;
        };
        xTaskCreate([](void *pvParam)
                    {
                    ReconnectParam *p = static_cast<ReconnectParam*>(pvParam);
                    WiFi.disconnect();
                    p->bundle->begin(*p->bundle->onProgress, *p->bundle->onTimeout);
                    
                    const uint32_t start = millis();
                    while (millis() - start < 30000) {
                        if (WiFi.status() == WL_CONNECTED) {
                            if (p->callback != nullptr) p->callback(true);
                            delete p;
                            vTaskDelete(nullptr);
                        }
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }
                    
                    if (p->callback != nullptr) p->callback(false);
                    delete p;
                    vTaskDelete(nullptr); },
                    "Reconnect", 2048,
                    new ReconnectParam{.bundle = this, .callback = onResult ? *onResult : std::function<void(bool)>()},
                    8, nullptr);
        return true;
    }
}

bool WiFiBundle::disconnect()
{
    return WiFi.disconnect();
}

int WiFiBundle::post(const char *url, const char *payload)
{
    int res;
    for (int i = 0; i < _maxRetry; i++)
    {
        int res = _transport->post(url, payload);
        if (res == 200 || res == 201)
            return res;
        delay(50);
    }
    return res;
}