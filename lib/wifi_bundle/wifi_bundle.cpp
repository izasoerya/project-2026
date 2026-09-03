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

void WiFiBundle::reconnect()
{
    const uint32_t currentTime = millis();
    while (millis() < currentTime + 5000)
        if (WiFi.status() == WL_CONNECTED)
            return;

    WiFi.disconnect(true); // force clean state
    delay(100);
    begin(*onProgress, *onTimeout);
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