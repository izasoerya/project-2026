#if !defined(MQTT_MODULE_H)
#define MQTT_MODULE_H

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <WebSocketsClient.h>
#include <MQTTPubSubClient.h>
#include <WiFiClientSecure.h>
#include "../base_transmitter.h"

class BaseMQTT
{
private:
    const char *_username;
    const char *_password;
    const char *_brokerUrl;
    const uint16_t _port;

protected:
    MQTTPubSubClient _mqttClient;
    bool _connectToMQTT(const char *username, const char *password, const char *id = "default-client")
    {
        _mqttClient.disconnect();
        uint32_t startAttempt = millis();
        startAttempt = millis();
        while (!_mqttClient.connect(id, username, password))
        {
            if (millis() - startAttempt > 5000) // 5 second before timeout
            {
                Serial.printf("Timeout connect to mqtt with username: %s, password: %s\n", username, password);
                return false;
            }
        }
        return true;
    }

public:
    BaseMQTT(
        const char *brokerUrl, const uint16_t port)
        : _brokerUrl(brokerUrl), _port(port) {}

    BaseMQTT(
        const char *brokerUrl, const uint16_t port,
        const char *username, const char *password)
        : _username(username), _password(password), _brokerUrl(brokerUrl), _port(port) {}

    ~BaseMQTT() = default;

    const char *getBrokerUrl() { return _brokerUrl; }
    uint16_t getPort() { return _port; }
    const char *getUsername() { return _username; }
    const char *getPassword() { return _password; }

    bool isConnected() { return _mqttClient.isConnected(); }
    bool disconnect() { return _mqttClient.disconnect(); }
    virtual bool poll() { return _mqttClient.update(); }

    virtual bool connect(const char *username, const char *password, const char *id = "default-client") = 0;

    virtual bool connect(const char *id = "default-client") = 0;

    void subscribe(const char *topic, std::function<void(const char *payload, const size_t size)> cb)
    {
        _mqttClient.subscribe(topic, cb);
    }

    uint16_t publish(const char *topic, const char *msg, bool retain = false, uint8_t qos = 0)
    {
        return _mqttClient.publish(topic, msg, retain, qos);
    }
};

class MQTTBasic : public BaseMQTT
{
private:
    WiFiClient _wClient;

public:
    MQTTBasic(
        const char *brokerUrl, const uint16_t port)
        : BaseMQTT(brokerUrl, port) {}

    MQTTBasic(
        const char *brokerUrl, const uint16_t port,
        const char *username, const char *password)
        : BaseMQTT(brokerUrl, port, username, password) {}

    ~MQTTBasic() = default;

    bool connect(const char *username, const char *password, const char *id = "default-client") override
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wClient.stop();
        uint32_t startAttempt = millis();
        while (!_wClient.connect(getBrokerUrl(), getPort()))
        {
            if (millis() - startAttempt > 5000) // 5 second before timeout
                return false;
        }
        _connectToMQTT(id, username, password);
    }

    bool connect(const char *id = "default-client")
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wClient.stop();
        uint32_t startAttempt = millis();
        while (!_wClient.connect(getBrokerUrl(), getPort()))
        {
            if (millis() - startAttempt > 5000) // 5 second before timeout
                return false;
        }
        _connectToMQTT(id, getUsername(), getPassword());
    }
};

class MQTTSecure : public BaseMQTT
{
private:
    WiFiClientSecure _wClient;

public:
    MQTTSecure(
        const char *brokerUrl, const uint16_t port)
        : BaseMQTT(brokerUrl, port)
    {
        _wClient.setInsecure();
    }

    MQTTSecure(
        const char *brokerUrl, const uint16_t port,
        const char *username, const char *password)
        : BaseMQTT(brokerUrl, port, username, password) {}

    ~MQTTSecure() = default;

    void setCert(const char *root_ca)
    {
        _wClient.setCACert(root_ca);
    }

    bool connect(const char *username, const char *password, const char *id = "default-client") override
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wClient.stop();
        uint32_t startAttempt = millis();
        while (!_wClient.connect(getBrokerUrl(), getPort()))
        {
            if (millis() - startAttempt > 5000) // 5 second before timeout
                return false;
        }
        return _connectToMQTT(id, username, password);
    }

    bool connect(const char *id = "default-client")
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wClient.stop();
        uint32_t startAttempt = millis();
        while (!_wClient.connect(getBrokerUrl(), getPort()))
        {
            if (millis() - startAttempt > 5000) // 5 second before timeout
                return false;
        }
        return _connectToMQTT(id, getUsername(), getPassword());
    }
};

class MQTTWebsocket : public BaseMQTT
{
private:
    WebSocketsClient _wsclient;

public:
    MQTTWebsocket(
        const char *brokerUrl, const uint16_t port)
        : BaseMQTT(brokerUrl, port) {}

    MQTTWebsocket(
        const char *brokerUrl, const uint16_t port,
        const char *username, const char *password)
        : BaseMQTT(brokerUrl, port, username, password) {}

    ~MQTTWebsocket() = default;

    bool connect(const char *username, const char *password, const char *id = "default-client") override
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wsclient.disconnect();
        _wsclient.begin(getBrokerUrl(), getPort(), "/mqtt", "mqtt");
        _wsclient.setReconnectInterval(2000);
        _mqttClient.begin(_wsclient);

        return _connectToMQTT(id, username, password);
    }

    bool connect(const char *id = "default-client")
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wsclient.disconnect();
        _wsclient.begin(getBrokerUrl(), getPort(), "/mqtt", "mqtt");
        _wsclient.setReconnectInterval(2000);
        _mqttClient.begin(_wsclient);

        return _connectToMQTT(id, getUsername(), getPassword());
    }

    bool poll() override
    {
        _wsclient.loop();
        return _mqttClient.update();
    }
};

class MQTTWebsocketSecure : public BaseMQTT
{
private:
    WebSocketsClient _wsclient;

public:
    MQTTWebsocketSecure(
        const char *brokerUrl, const uint16_t port)
        : BaseMQTT(brokerUrl, port) {}

    MQTTWebsocketSecure(
        const char *brokerUrl, const uint16_t port,
        const char *username, const char *password)
        : BaseMQTT(brokerUrl, port, username, password) {}

    ~MQTTWebsocketSecure() = default;

    bool connect(const char *username, const char *password, const char *id = "default-client") override
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wsclient.disconnect();
        // TODO: METHOD CONNECT TO WS WITH CERT
        _wsclient.beginSSL(getBrokerUrl(), getPort(), "/mqtt", "", "mqtt");
        _wsclient.setReconnectInterval(2000);
        _mqttClient.begin(_wsclient);

        return _connectToMQTT(username, password, id);
    }

    bool connect(const char *id = "default-client")
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        _wsclient.disconnect();
        // TODO: METHOD CONNECT TO WS WITH CERT
        _wsclient.beginSSL(getBrokerUrl(), getPort(), "/mqtt", "", "mqtt");
        _wsclient.setReconnectInterval(2000);
        _mqttClient.begin(_wsclient);

        return _connectToMQTT(id, getUsername(), getPassword());
    }

    bool poll() override
    {
        _wsclient.loop();
        return _mqttClient.update();
    }
};

#endif // MQTT_MODULE_H