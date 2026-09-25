#if !defined(MQTT_MODULE_H)
#define MQTT_MODULE_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "../base_transmitter.h"

class MQTTModule
{
private:
    static const uint8_t _qos = 0;
    static const bool _retain = false;

    const char *_username;
    const char *_password;
    const char *_brokerUrl;
    const uint16_t _port;

    WiFiClientSecure _wifiClient;
    PubSubClient _mqttClient;

    void (*_userCallback)(const char *topic, const char *payload) = nullptr;

public:
    MQTTModule(
        const char *username, const char *password, const char *brokerUrl, const uint16_t port)
        : _username(username), _password(password), _brokerUrl(brokerUrl), _port(port),
          _mqttClient(_wifiClient)
    {
        _wifiClient.setInsecure();
        _mqttClient.setServer(_brokerUrl, _port);
    }
    ~MQTTModule() = default;

    bool isConnected() { return _mqttClient.connected(); }

    bool connect()
    {
        bool result = _mqttClient.connect("ESP32Client", _username, _password);
        if (!result)
        {
            Serial.printf("MQTT connect failed. State: %d\n", _mqttClient.state());
            // State codes: -4=CONNECT_FAILED, -3=CONNECT_BAD_PROTOCOL, -2=CONNECT_BAD_CLIENT_ID
            //              -1=CONNECT_UNAVAILABLE, 0=CONNECT_BAD_CREDENTIALS, 1=CONNECTED
        }
        return result;
    }

    bool reconnect()
    {
        if (_mqttClient.connect("ESP32Client", _username, _password))
            return true;
        return false;
    }

    bool disconnect()
    {
        _mqttClient.disconnect();
        return true;
    }

    void onMessage(void (*_msgCallback)(const char *topic, const char *payload))
    {
        _userCallback = _msgCallback;
        _mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                                {
            char buffer[length + 1];
            memcpy(buffer, payload, length);
            buffer[length] = '\0';
            if (_userCallback)
                _userCallback(topic, buffer); });
    }

    uint16_t publish(const char *topic, const char *msg)
    {
        return _mqttClient.publish(topic, msg, _retain) ? 1 : 0;
    }
};

#endif // MQTT_MODULE_H