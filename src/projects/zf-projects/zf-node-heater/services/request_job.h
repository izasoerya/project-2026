#if !defined(REQUEST_JOB_H)
#define REQUEST_JOB_H

#include <asyncHTTPrequest.h>
#include <ArduinoJson.h>

#include "../models/sensor.h"
#include "../models/actuator.h"
#include "../models/actuator_mode.h"

enum RequestType
{
    SENSOR = 0,
    ACTUATOR,
    ACTUATOR_MODE,
    TELEGRAM
};

enum RequestState
{
    WAITING = 0,
    DONE
};

class RequestJob
{
private:
    const char *_floorIds[3] = {
        "433b141d-b7db-415f-86e0-c42f322dbeff",
        "433b141d-b7db-415f-86e0-c42f322dbefg",
        "433b141d-b7db-415f-86e0-c42f322dbefh",
    };
    const char *_serverAddress = "panel-control-v4-pd1.local:8000";
    char _urlSensorReq[128];
    char _urlActuatorReq[128];
    char _urlActuatorModeReq[128];
    char _urlTelegramReq[128];

    Sensor _latestSensor;
    Actuator _latestActuator;
    ActuatorMode _latestActuatorMode;
    const char *_latestUrl;

    RequestType _reqType = RequestType::SENSOR;
    RequestState _reqState = RequestState::DONE;
    uint8_t retriesSensor = 0;
    uint8_t retriesActuator = 0;
    uint8_t retriesActuatorMode = 0;

    asyncHTTPrequest _request;
    std::function<void(Actuator)> _cb;
    std::function<void()> _cbTimeout = nullptr;

public:
    RequestJob(uint8_t floor, std::function<void(Actuator)> cb, std::function<void()> timeoutCb) : _cb(cb), _cbTimeout(timeoutCb)
    {
        snprintf(_urlSensorReq, sizeof(_urlSensorReq),
                 "http://%s/sensor/find-latest/%s", _serverAddress, _floorIds[floor - 1]);
        snprintf(_urlActuatorReq, sizeof(_urlActuatorReq),
                 "http://%s/actuator/find-latest/%s", _serverAddress, _floorIds[floor - 1]);
        snprintf(_urlActuatorModeReq, sizeof(_urlActuatorModeReq),
                 "http://%s/actuator-mode/find-latest/%s", _serverAddress, _floorIds[floor - 1]);
    }
    ~RequestJob() {}

    void begin()
    {
        _request.onReadyStateChange(_cbRequest, (void *)this);
    }

    void requestSensorData()
    {
        if (_request.readyState() == 0 || _request.readyState() == 4)
        {
            if (_request.open("GET", _urlSensorReq))
            {
                _request.send();
                _latestUrl = _urlSensorReq;
                _reqState = RequestState::WAITING;
                _reqType = RequestType::SENSOR;
            }
        }
    }

    void requestActuatorData()
    {
        if (_request.readyState() == 0 || _request.readyState() == 4)
        {
            if (_request.open("GET", _urlActuatorReq))
            {
                _request.send();
                _latestUrl = _urlActuatorReq;
                _reqState = RequestState::WAITING;
                _reqType = RequestType::ACTUATOR;
            }
        }
    }

    void requestActuatorModeData()
    {
        if (_request.readyState() == 0 || _request.readyState() == 4)
        {
            if (_request.open("GET", _urlActuatorModeReq))
            {
                _request.send();
                _latestUrl = _urlActuatorModeReq;
                _reqState = RequestState::WAITING;
                _reqType = RequestType::ACTUATOR_MODE;
            }
        }
    }

    void postTelegramNotification()
    {
        if (_request.open("POST", _urlTelegramReq))
            _request.send("Message");
        _reqType = RequestType::TELEGRAM;
    }

    void _handleResponse(asyncHTTPrequest *request)
    {
        _reqState = RequestState::DONE;
        if (request->responseHTTPcode() == 200)
        {
            String response = request->responseText();
            Serial.printf("Response: %s\n", response.c_str());

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);
            if (error)
            {
                Serial.printf("JSON parse error: %s\n", error.c_str());
                return;
            }

            if (_reqType == RequestType::SENSOR)
            {
                retriesSensor = 0;
                Sensor sensorObject{
                    .id = doc["id"],
                    .floorId = doc["floor_entity_id"],
                    .temperature = doc["temperature"]};
                _latestSensor = sensorObject;
            }
            else if (_reqType == RequestType::ACTUATOR)
            {
                retriesActuator = 0;
                JsonArray heaterArray = doc["heater_value"].as<JsonArray>();
                bool heaterValue = heaterArray[0].as<bool>();

                Actuator actuatorObject{
                    .id = doc["id"],
                    .floorId = doc["floor_entity_id"],
                    .state = heaterValue};
                _latestActuator = actuatorObject;
            }
            else if (_reqType == RequestType::ACTUATOR_MODE)
            {
                retriesActuatorMode = 0;
                ActuatorMode modeObject{
                    .id = doc["id"],
                    .floorId = doc["floor_entity_id"],
                    .topTemperature = doc["top_temperature_target_heater"],
                    .botTemperature = doc["bottom_temperature_target_heater"]};
                _latestActuatorMode = modeObject;
            }
            _cb(_latestActuator);
        }
        else
        {
            if (_reqType == RequestType::ACTUATOR)
            {
                if (retriesActuator < 5)
                {
                    delay(250);
                    requestActuatorData();
                }
                else
                {
                    if (_cbTimeout != nullptr)
                        (_cbTimeout)();
                    Serial.printf("Retries Actuator Timeout");
                }
                retriesActuator++;
                Serial.printf("Request Actuator Error: %d| Retrying...\n", request->responseHTTPcode());
            }
            if (_reqType == RequestType::ACTUATOR_MODE)
            {
                if (retriesActuatorMode < 5)
                {
                    delay(250);
                    requestActuatorModeData();
                }
                retriesActuatorMode++;
                Serial.printf("Request Actuator Mode Error: %d| Retrying...\n", request->responseHTTPcode());
            }
            if (_reqType == RequestType::SENSOR)
            {
                if (retriesSensor < 5)
                {
                    delay(250);
                    requestSensorData();
                }
                retriesSensor++;
                Serial.printf("Request Sensor Error: %d| Retrying...\n", request->responseHTTPcode());
            }
        }
    }

    static void _cbRequest(void *optParm, asyncHTTPrequest *request, int readyState)
    {
        if (readyState == 4)
        {
            RequestJob *self = (RequestJob *)optParm;
            self->_handleResponse(request);
        }
    }
};

#endif // REQUEST_JOB_H
