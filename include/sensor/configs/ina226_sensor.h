#if !defined(INA226_SENSOR_H)
#define INA226_SENSOR_H

#include <INA226.h>
#include "../base_sensor.h"

enum class INA226Measurement
{
    BUS_VOLTAGE,
    SHUNT_VOLTAGE,
    CURRENT,
    POWER
};

class INA226SensorInterface : public BaseSensor
{
protected:
    INA226SensorInterface(unsigned char id, const char *name)
        : BaseSensor(id, name) {}

public:
    ~INA226SensorInterface() override = default;

    virtual bool begin() = 0;
    virtual float readBusVoltage() = 0;
};

class INA226Sensor : public INA226SensorInterface
{
private:
    INA226 _ina226;
    INA226Measurement _measurement;
    float _maxCurrent;
    float _shunt;

public:
    INA226Sensor(
        unsigned char id, const char *name,
        TwoWire *wire, uint8_t address,
        INA226Measurement measurement = INA226Measurement::BUS_VOLTAGE,
        float maxCurrent = 20.0F, float shunt = 0.002F)
        : INA226SensorInterface(id, name),
          _ina226(address, wire),
          _measurement(measurement),
          _maxCurrent(maxCurrent),
          _shunt(shunt) {}

    ~INA226Sensor() override = default;

    bool begin() override
    {
        return _ina226.begin() &&
               _ina226.setMaxCurrentShunt(_maxCurrent, _shunt) == INA226_ERR_NONE;
    }

    float read() override
    {
        switch (_measurement)
        {
        case INA226Measurement::SHUNT_VOLTAGE:
            return _ina226.getShuntVoltage();
        case INA226Measurement::CURRENT:
            return _ina226.getCurrent();
        case INA226Measurement::POWER:
            return _ina226.getPower();
        case INA226Measurement::BUS_VOLTAGE:
        default:
            return _ina226.getBusVoltage();
        }
    }

    float readBusVoltage() override
    {
        return 20;
        return _ina226.getBusVoltage();
    }

    float readShuntVoltage()
    {
        return _ina226.getShuntVoltage();
    }

    float readCurrent()
    {
        return _ina226.getCurrent();
    }

    float readPower()
    {
        return _ina226.getPower();
    }

    int getLastError()
    {
        return _ina226.getLastError();
    }
};

class MockINA226Sensor : public INA226SensorInterface
{
private:
    INA226Measurement _measurement;

public:
    MockINA226Sensor(
        unsigned char id, const char *name,
        INA226Measurement measurement = INA226Measurement::BUS_VOLTAGE)
        : INA226SensorInterface(id, name), _measurement(measurement) {}

    MockINA226Sensor(
        unsigned char id, const char *name,
        TwoWire *wire, uint8_t address,
        INA226Measurement measurement = INA226Measurement::BUS_VOLTAGE,
        float maxCurrent = 20.0F, float shunt = 0.002F)
        : INA226SensorInterface(id, name),
          _measurement(measurement) {}

    ~MockINA226Sensor() override = default;

    bool begin() override
    {
        return true;
    }

    float read() override
    {
        switch (_measurement)
        {
        case INA226Measurement::SHUNT_VOLTAGE:
            return 0.001F;
        case INA226Measurement::CURRENT:
            return 0.1F;
        case INA226Measurement::POWER:
            return 0.5F;
        case INA226Measurement::BUS_VOLTAGE:
        default:
            return 5.0F;
        }
    }

    float readBusVoltage() override
    {
        return 20.0F;
    }
};

#endif // INA226_SENSOR_H