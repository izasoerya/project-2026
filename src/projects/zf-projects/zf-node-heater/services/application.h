#if !defined(APPLICATION_H)
#define APPLICATION_H

#include <Arduino.h>
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
};

#endif // APPLICATION_H
