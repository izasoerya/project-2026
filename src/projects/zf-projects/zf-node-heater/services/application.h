#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <Arduino.h>
#include "projects/zf-projects/zf-node-heater/models/actuator.h"
#include "projects/zf-projects/zf-node-heater/consts/global_definition.h"
#include "projects/zf-projects/zf-node-heater/models/task_context.h"

class Application
{
public:
    static void IRAM_ATTR buttonISR(void *pvParam)
    {
        ActuatorDriverContext *ctx = static_cast<ActuatorDriverContext *>(pvParam);

        static uint32_t lastButtonTime = millis();
        if (millis() - lastButtonTime > 50)
        {
            if (digitalRead(GlobalConfig::PIN_BUTTON) == LOW)
            {
                ctx->driver.controlActuator(!ctx->driver.getActuatorState());
                lastButtonTime = millis();
            }
        }
    }

    static void automationActuatorCb(Actuator actuator, void *pvParam)
    {
        ActuatorDriverContext *ctx = static_cast<ActuatorDriverContext *>(pvParam);

        bool prevData = actuator.state;
        ctx->driver.controlActuator(prevData);
        bool actualValue = digitalRead(GlobalConfig::PIN_HEATER_RELAY);
        if (prevData != actualValue)
        {
            Serial.println("Discrepancy output detected!");
            WebSerial.println("Discrepancy output detected!");
        }

        Serial.printf("RELAY: %s\n", actualValue ? "ON" : "OFF");
        WebSerial.printf("RELAY: %s\n", actualValue ? "ON" : "OFF");
    }

    static void timeoutActautorCb(void *pvParam)
    {
        AutomationActuatorContext *ctx = static_cast<AutomationActuatorContext *>(pvParam);

        static char buffer[64];
        snprintf(buffer, sizeof(buffer), "Node Heater-%d Timeout Fetch Actuator", GlobalConfig::FLOOR_ID);

        Serial.println(buffer);
        WebSerial.println(buffer);
        if (ctx->mqtt.isConnected())
            ctx->mqtt.publish("TELEGRAM", buffer);
    }
};

#endif // APPLICATION_H
