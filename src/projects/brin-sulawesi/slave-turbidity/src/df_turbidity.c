#include "df_turbidity.h"

int16_t read_turbidity()
{
    uint32_t adc_accumulator = 0;

    // 1. Oversample the ADC (16 times)
    for (uint8_t i = 0; i < NUM_SAMPLES; i++)
    {
        adc_accumulator += analogRead(TURBIDITY_PIN);
        delay(1);
    }

    // Fast integer division using bitshift (Equivalent to / 16)
    uint16_t average_adc = adc_accumulator >> 4;

    // Prevent out-of-bounds array access and handle hardware faults
    if (average_adc < 16 || average_adc >= 1008)
    {
        return -999;
    }

    return ((-1120 * average_adc * average_adc * 100) + (5742 * average_adc * 100) - (4352 * 100)) / 10;
}