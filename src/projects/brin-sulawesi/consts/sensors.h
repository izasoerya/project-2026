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
    Modbustatics(0, "awlr-1", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(1, "awlr-2", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(2, "awlr-3", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(3, "awlr-4", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(4, "awlr-5", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(5, "awlr-6", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(6, "awlr-7", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(7, "awlr-8", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
    Modbustatics(8, "awlr-9", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),
};

static Modbustatics turb_list[9] = {
    Modbustatics(0, "turb-ln-1", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v;  
    }),

    Modbustatics(1, "turb-ln-2", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(2, "turb-ln-3", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(3, "turb-df-1", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(4, "turb-df-2", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(5, "turb-df-3", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(6, "turb-df-4", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(7, "turb-df-5", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    }),
    
    Modbustatics(8, "turb-df-6", Serial2, 1, [](float v) { 
        static MovingAverageFilter mvFilter(20);
        mvFilter.filter(v);
        return v; 
    })
};

#endif // SENSOR_LIST_H
