#include "thermistor.h"

int16_t read_thermistor_temp()
{
    uint32_t adc_accumulator = 0;

    // 1. Oversample the ADC (16 times)
    for (uint8_t i = 0; i < NUM_SAMPLES; i++)
    {
        adc_accumulator += analogRead(THERMISTOR_PIN);
        delay(1);
    }

    // Fast integer division using bitshift (Equivalent to / 16)
    uint16_t average_adc = adc_accumulator >> 4;

    // Prevent out-of-bounds array access and handle hardware faults
    if (average_adc < 16 || average_adc >= 1008)
    {
        return -999;
    }

    // 2. Linear Interpolation
    // Find the two nearest pre-calculated points in the LUT
    uint8_t index = average_adc >> 5;       // Equivalent to average_adc / 32
    uint8_t remainder = average_adc & 0x1F; // Equivalent to average_adc % 32

    int16_t temp_low = thermistor_lut[index];
    int16_t temp_high = thermistor_lut[index + 1];

    // Calculate exact temperature between the two LUT points
    // Formula: temp = temp_low + ((temp_high - temp_low) * remainder) / 32
    int16_t diff = temp_high - temp_low;
    int16_t interpolated_temp = temp_low + ((diff * remainder) / 32);

    return interpolated_temp;
}