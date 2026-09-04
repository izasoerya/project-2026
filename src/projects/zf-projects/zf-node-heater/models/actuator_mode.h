#if !defined(MODELS_ACTUATOR_MODE)
#define MODELS_ACTUATOR_MODE

struct ActuatorMode
{
    unsigned long id;
    unsigned char floorId;
    float topTemperature;
    float botTemperature;
};

#endif // MODELS_ACTUATOR_MODE
