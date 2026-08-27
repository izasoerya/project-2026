#if !defined(MQTT_MODULE_H)
#define MQTT_MODULE_H

#include <WiFi.h>
#include <espMqttClient.h>

class MQTTModule
{
private:
    static const uint8_t _qos = 0;
    static const bool _retain = false;
    static const uint16_t _port = 1883;

    const char *_username;
    const char *_password;
    const char *_brokerUrl;

    espMqttClient _mqttClient;

public:
    MQTTModule(
        const char *username, const char *password, const char *brokerUrl)
        : _username(username), _password(password), _brokerUrl(brokerUrl) {}

    ~MQTTModule() {}

    bool connect()
    {
        _mqttClient.setServer(_brokerUrl, _port);
        if (!_mqttClient.connect())
            return false;
        else
            return true;
    }

    void onMessage(void (*_msgCallback)(const char *topic, const char *payload))
    {
        _mqttClient.onMessage(
            [&](const espMqttClientTypes::MessageProperties &properties,
                const char *topic, const uint8_t *payload, size_t len,
                size_t index, size_t total)
            {
                char buffer[len + 1];
                memcpy(buffer, payload, len);
                buffer[len] = '\0';
                _msgCallback(topic, buffer);
            });
    }

    uint16_t publish(const char *topic, const char *msg)
    {
        return _mqttClient.publish(topic, _qos, _retain, msg); // return packet id, packet is 0 if fail
    }
};

#endif // MQTT_MODULE_H
