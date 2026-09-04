#if !defined(MODELS_ACTUATOR_H)
#define MODELS_ACTUATOR_H

struct Actuator
{
    unsigned long id;
    const char *floorId;
    bool state;
};

#endif // MODELS_ACTUATOR_H
