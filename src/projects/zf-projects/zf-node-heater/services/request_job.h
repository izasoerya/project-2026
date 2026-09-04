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
    const char *_urlSensorReq = "http://172.29.183.12:8000/sensor/find-latest/433b141d-b7db-415f-86e0-c42f322dbeff";
    const char *_urlActuatorReq = "http://172.29.183.12:8000/actuator/find-latest/433b141d-b7db-415f-86e0-c42f322dbeff";
    const char *_urlActuatorModeReq = "http://172.29.183.12:8000/actuator-mode/find-latest/433b141d-b7db-415f-86e0-c42f322dbeff";
    const char *_urlTelegramReq = "telegram_url";

    Sensor _latestSensor;
    Actuator _latestActuator;
    ActuatorMode _latestActuatorMode;
    const char *_latestUrl;

    RequestType _reqType = RequestType::SENSOR;
    RequestState _reqState = RequestState::DONE;

    asyncHTTPrequest _request;
    std::function<void(Actuator)> _cb;

public:
    RequestJob(std::function<void(Actuator)> cb) : _cb(cb) {}
    ~RequestJob() {}

    void begin()
    {
        _request.onReadyStateChange(cbRequest, (void *)this);
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

    void handleResponse(asyncHTTPrequest *request)
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
                Sensor sensorObject{
                    .id = doc["id"],
                    .floorId = doc["floor_entity_id"],
                    .temperature = doc["temperature"]};
                _latestSensor = sensorObject;
            }
            else if (_reqType == RequestType::ACTUATOR)
            {
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
            Serial.printf("Req Error: %d\n", request->responseHTTPcode());
    }

    static void cbRequest(void *optParm, asyncHTTPrequest *request, int readyState)
    {
        if (readyState == 4)
        {
            RequestJob *self = (RequestJob *)optParm;
            self->handleResponse(request);
        }
    }
};

#endif // REQUEST_JOB_H
