// clang-format off

#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

#include "sensor/configs/modbus_sensor.h"
#include "sensor/filters/moving_average.h"

/**
 * @brief TODO: Create a protocol to send calibration to modbus slave device
 * 
 * 1. Create minimum program for reading without calibration
 * 2. Prepare protocol sending data (id, data, len, crc)
 * 3. Prepare protocol calibration
 */

static Modbustatics awlr_list[9] = {
    Modbustatics(1, "awlr-1", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(2, "awlr-2", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(3, "awlr-3", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(4, "awlr-4", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(5, "awlr-5", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(6, "awlr-6", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(7, "awlr-7", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(8, "awlr-8", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(9, "awlr-9", GlobalConfig::mbSerial, 0x100, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
};

static Modbustatics turb_list[9] = {
    Modbustatics(11, "turb-ln-1", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),

    Modbustatics(12, "turb-ln-2", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(13, "turb-ln-3", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(14, "turb-df-1", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(15, "turb-df-2", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(16, "turb-df-3", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(17, "turb-df-4", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(18, "turb-df-5", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(19, "turb-df-6", GlobalConfig::mbSerial, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    })
};

#endif // SENSOR_LIST_H
