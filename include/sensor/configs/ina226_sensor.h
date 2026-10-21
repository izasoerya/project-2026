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

class INA226Sensor : public BaseSensor
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
        : BaseSensor(id, name),
          _ina226(address, wire),
          _measurement(measurement),
          _maxCurrent(maxCurrent),
          _shunt(shunt) {}

    ~INA226Sensor() override = default;

    bool begin()
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

    float readBusVoltage()
    {
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

class MockINA226Sensor : public BaseSensor
{
private:
    INA226Measurement _measurement;

public:
    MockINA226Sensor(
        unsigned char id, const char *name,
        INA226Measurement measurement = INA226Measurement::BUS_VOLTAGE)
        : BaseSensor(id, name), _measurement(measurement) {}

    ~MockINA226Sensor() override = default;

    bool begin()
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
};

#endif // INA226_SENSOR_H