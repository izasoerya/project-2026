// clang-format off

#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

#include "sensor/configs/modbus_sensor.h"
#include "sensor/configs/ina226_sensor.h"
#include "slave_registers.h"

// Switch this alias to MockINA226Sensor for hardware-independent testing.
using SelectedINA226Sensor = INA226Sensor;

/**
 * @brief TODO: Create a protocol to send calibration to modbus slave device
 * 
 * 1. Create minimum program for reading without calibration
 * 2. Prepare protocol sending data (id, data, len, crc)
 * 3. Prepare protocol calibration
 */

static Modbustatics arr_list[4] = {
    Modbustatics(1, "arr-1", GlobalConfig::mbSerial, ModbusRegister::TIP_DAY, [](float v) { 
        return v;  
    }),
    Modbustatics(2, "arr-2", GlobalConfig::mbSerial, ModbusRegister::TIP_DAY, [](float v) { 
        return v;  
    }),
    Modbustatics(3, "arr-3", GlobalConfig::mbSerial, ModbusRegister::TIP_DAY, [](float v) { 
        return v;  
    }),
    Modbustatics(4, "arr-4", GlobalConfig::mbSerial, ModbusRegister::TIP_DAY, [](float v) { 
        return v;  
    })
};

static SelectedINA226Sensor ina_accu_list[4] = {
    SelectedINA226Sensor (1, "Accu voltage & current 1", &Wire, 0x04, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (2, "Accu voltage & current 2", &Wire, 0x04, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (3, "Accu voltage & current 3", &Wire, 0x04, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (4, "Accu voltage & current 4", &Wire, 0x04, INA226Measurement::BUS_VOLTAGE)
};

static SelectedINA226Sensor ina_solar_list[4] = {
    SelectedINA226Sensor (1, "Solar panel voltage & current", &Wire, 0x05, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (2, "Solar panel voltage & current", &Wire, 0x05, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (3, "Solar panel voltage & current", &Wire, 0x05, INA226Measurement::BUS_VOLTAGE),
    SelectedINA226Sensor (4, "Solar panel voltage & current", &Wire, 0x05, INA226Measurement::BUS_VOLTAGE)
};



#endif // SENSOR_LIST_H
