#if !defined(DF_TURBIDITY_H)
#define DF_TURBIDITY_H

#include <Arduino.h>

#define TURBIDITY_PIN PD2
#define NUM_SAMPLES 16 // Power of 2 allows bit-shift division

/* ADC reference voltage in millivolts (STM8 VDD). Adjust if a divider / 3V3 supply is used. */
#define TURBIDITY_ADC_VREF_MV 5000

/* Returned by the turbidity conversions when the averaged ADC is out of the valid window */
#define TURBIDITY_FAULT -999

/* DFRobot curve is only valid between 0 and 3000 NTU */
#define TURBIDITY_MAX_NTU_X10 30000

/**
 * @brief Read the turbidity sensor ADC (16x oversampled average, 0..1023).
 */
uint16_t read_turbidity_raw(void);

/**
 * @brief Convert raw ADC to NTU x10 using the DFRobot SEN0189 curve:
 *        NTU = -1120.4*V^2 + 5742.3*V - 4352.9  (V in volts)
 * Result is clamped to 0..3000.0 NTU. Returns TURBIDITY_FAULT on sensor fault.
 */
int16_t turbidity_dfrobot_x10(uint16_t raw);

/**
 * @brief Convert raw ADC to NTU x10 using a user calibration y = m*x + c.
 * @param m_x1000 slope scaled by 1000 (e.g. 1000 = 1.000 NTU per ADC count)
 * @param c_x10   offset scaled by 10 (NTU x10)
 * Returns TURBIDITY_FAULT on sensor fault.
 */
int16_t turbidity_calibrated_x10(uint16_t raw, int16_t m_x1000, int16_t c_x10);

#endif // DF_TURBIDITY_H
