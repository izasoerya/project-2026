#if !defined(TASK_CONTEXT_H)
#define TASK_CONTEXT_H

#include "projects/zf-projects/zf-node-heater/services/actuator_driver.h"

struct ActuatorDriverContext
{
    ActuatorDriver &driver;

    ActuatorDriverContext(ActuatorDriver &d) : driver(d) {}
};

#endif // TASK_CONTEXT_H
