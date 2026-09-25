#if !defined(APPLICATION_H)
#define APPLICATION_H

#include "../consts/sensors.h"
#include "../models/task_context.h"
#include "../models/datastore.h"

struct MqttPayload
{
    const char *topic;
    const char *message;
};

class Application
{
public:
    static bool networkingTask(void *pvParam, MqttPayload payload)
    {
        NetworkingContext *ctx = static_cast<NetworkingContext *>(pvParam);
        while (WiFi.status() != WL_CONNECTED)
            ctx->wifi.begin(false); // BLOCKING TIMEOUT 20S

        MQTTModule mqtt(
            GlobalConfig::usernameMqtt,
            GlobalConfig::passwordMqtt,
            GlobalConfig::brokerMqtt,
            GlobalConfig::portMqtt);
        bool isMQTTConnected = mqtt.connect(); // BLOCKING TIMEOUT 20S
        if (mqtt.connect())
            return mqtt.publish(payload.topic, payload.message) != 0;
        return false;
    }

    static SensorObject sensorAgregatorTask(SensorObject sensor, bool resetBuffer)
    {
        static SensorObject sensorDatapoints[32];
        static uint8_t arrayPointer = 0;

        if (!resetBuffer && arrayPointer < 32)
            sensorDatapoints[arrayPointer++] = sensor;

        float avgTurbidity = 0.0f;
        float avgAwlr = 0.0f;
        float avgBattery = 0.0f;
        for (uint8_t i = 0; i < arrayPointer; ++i)
        {
            avgTurbidity += sensorDatapoints[i].turbidity;
            avgAwlr += sensorDatapoints[i].awlr;
            avgBattery += sensorDatapoints[i].battery;
        }
        SensorObject avgSensor = {0.0f, 0.0f, 0.0f};
        if (arrayPointer > 0)
        {
            avgSensor = SensorObject{
                .turbidity = avgTurbidity / arrayPointer,
                .awlr = avgAwlr / arrayPointer,
                .battery = avgBattery / arrayPointer,
            };
        }

        if (resetBuffer)
        {
            memset(sensorDatapoints, 0, sizeof(sensorDatapoints));
            arrayPointer = 0;
        }
        return avgSensor;
    }

    static SensorObject samplingTask(void *pvParam)
    {
        SensorContext *ctx = static_cast<SensorContext *>(pvParam);

        // ReadResult resTurbidity = ctx->turbidity.rawRead();
        // ReadResult resAwlr = ctx->awlr.rawRead();
        // float batteryRead = analogRead(A10);

        // float turbidity, waterLevel;
        // if (resTurbidity.isOk())
        //     turbidity = resTurbidity.value;
        // if (resAwlr.isOk())
        //     waterLevel = resAwlr.value;

        static SensorObject sensor = SensorObject{
            .turbidity = 10,
            .awlr = 22,
            .battery = 2.3};
        return sensor;
    }

    static void calibrateTask(void *pvParam)
    {
        const size_t MAX_WINDOW = 30;
        uint8_t counter = 0;
        uint16_t val[MAX_WINDOW];

        ApplicationContext *ctx = static_cast<ApplicationContext *>(pvParam);
        while (1)
        {
            if (ctx->state == AppState::CALIBRATE_TURBIDITY)
            {
                val[counter] = ctx->mbTurbidity->read();
                counter++;
            }
            else if (ctx->state == AppState::CALIBRATE_ARR)
            {
                val[counter] = ctx->mbAwlr->read();
                counter++;
            }

            if (counter >= 30)
            {
                Serial.print("array = [");
                for (uint8_t i = 0; i < MAX_WINDOW; i++)
                    Serial.printf("%d, ", val[i]);
                Serial.println("]");
                counter = 0;
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
};

#endif // APPLICATION_H
