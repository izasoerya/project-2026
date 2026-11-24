#if !defined(DF_TURBIDITY_H)
#define DF_TURBIDITY_H

#include <Arduino.h>

#define TURBIDITY_PIN PD2
#define NUM_SAMPLES 16 // Power of 2 allows bit-shift division

int16_t read_turbidity();

#endif // DF_TURBIDITY_H
