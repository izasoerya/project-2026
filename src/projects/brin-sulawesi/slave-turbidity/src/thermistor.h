#if !defined(THERMISTOR_H)
#define THERMISTOR_H

#include <Arduino.h>

#define THERMISTOR_PIN 15
#define NUM_SAMPLES 16 // Power of 2 allows bit-shift division

// Pre-calculated Lookup Table for 10k NTC, B=3950, 10k Series Resistor.
// Mapped to 10-bit ADC steps of 32 (0, 32, 64, 96 ... 1024).
// Values represent Temperature * 10 (e.g., 250 = 25.0 C).
static const int16_t thermistor_lut[33] = {
    9999,                                               // Index 0: ADC 0 (Short to GND Error)
    1343, 1079, 924, 814, 728, 658, 598, 546, 500, 458, // Indices 1-10
    419, 383, 349, 318, 287, 258, 229, 201, 173, 144,   // Indices 11-20
    115, 85, 53, 19, -16, -56, -102, -159, -234, -345,  // Indices 21-30
    -9999, -9999                                        // Indices 31-32: ADC 992, 1024 (Open Circuit Error)
};

int16_t read_thermistor_temp();

#endif // THERMISTOR_H