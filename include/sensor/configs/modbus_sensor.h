#if !defined(MODBUS_SENSOR_H)
#define MODBUS_SENSOR_H

#include <Arduino.h>
#include "SensorBuilder.h"
#include "../base_sensor.h"
#include <sensor/filters/kalman_filter.h>

class ModbusSensor : public BaseSensor
{
private:
    ModbusRTUBuilder _modbusConfig;
    BaseFilter *_filter;

public:
    ModbusSensor(
        unsigned char id, const char *name,
        Stream *stream,
        BaseFilter *filter = nullptr)
        : BaseSensor(id, name),
          _modbusConfig(ModbusRTUBuilder(*stream)),
          _filter(filter) {}

    ~ModbusSensor() override = default;

    bool begin()
    {
        ReadResult res = _modbusConfig.connect();
        if (res.isOk())
            return 1;
        return 0;
    }

    ModbusRTUBuilder &build()
    {
        return _modbusConfig;
    }

    float read() override
    {
        ReadResult res = _modbusConfig.read(0);
        if (!res.isOk())
        {
            Serial.println(res.errorMessage());
            return res.error;
        }
        else
        {
            float raw = float(_modbusConfig.read(0).value);
            if (_filter != nullptr)
                _filter->filter(raw);

            return raw;
        }
    }
};

class Modbustatics : public BaseSensor
{
private:
    const uint8_t _address;
    ModbusRTUBuilder _modbusConfig;
    const std::function<float(float)> &_interceptor;

public:
    Modbustatics(
        unsigned char id, const char *name,
        Stream &stream, uint8_t address,
        const std::function<float(float)> &interceptor = nullptr)
        : BaseSensor(id, name),
          _address(address),
          _modbusConfig(ModbusRTUBuilder(stream)),
          _interceptor(interceptor) {}

    ~Modbustatics() override = default;

    unsigned char getId() { return getId(); }

    void begin()
    {
        _modbusConfig.setSlaveId(1).setFunctionCode(0x03).setAddress(_address).setLengthAddress(1);
    }

    float read() override
    {
        return 20;
        ReadResult res = _modbusConfig.read(0);
        if (res.isOk())
        {
            if (_interceptor != nullptr)
                return _interceptor(res.value);
            return res.value;
        }
        else
            return res.error; // TODO: SHOULD RETURN ACTUAL ERROR INSTEAD OF NUMBER
    }
};

#endif // MODBUS_SENSOR_H
