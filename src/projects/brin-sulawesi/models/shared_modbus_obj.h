#if !defined(SHARED_MODBUS_OBJECT_H)
#define SHARED_MODBUS_OBJECT_H

#include <freertos/semphr.h>
#include <sensor/configs/modbus_sensor.h>
#include "../services/appstate_parser.h"

struct ApplicationContext
{
    volatile AppState state;
    Modbustatics *mbTurbidity;
    Modbustatics *mbAwlr;
    SemaphoreHandle_t mutex;

    ApplicationContext()
    {
        state = AppState::NORMAL;
        mbTurbidity = nullptr;
        mbAwlr = nullptr;
        mutex = xSemaphoreCreateMutex();
    }
};

#endif // SHARED_MODBUS_OBJECT_H