#include "df_turbidity.h"

static uint8_t turbidity_is_fault(uint16_t raw)
{
    /* Near 0 V = disconnected / shorted, near VREF = saturated */
    return raw < 16 || raw >= 1008;
}

static int16_t clamp_i16(int32_t value)
{
    if (value > 32767)
        return 32767;
    if (value < -32768)
        return -32768;
    return (int16_t)value;
}

uint16_t read_turbidity_raw(void)
{
    uint32_t adc_accumulator = 0;

    for (uint8_t i = 0; i < NUM_SAMPLES; i++)
    {
        adc_accumulator += analogRead(TURBIDITY_PIN);
        delay(1);
    }

    return (uint16_t)(adc_accumulator >> 4); // Equivalent to / 16
}

int16_t turbidity_dfrobot_x10(uint16_t raw)
{
    if (turbidity_is_fault(raw))
        return TURBIDITY_FAULT;

    /* Integer-only evaluation, all intermediates fit in int32 (mV <= 5000) */
    int32_t mv = ((int32_t)raw * TURBIDITY_ADC_VREF_MV) >> 10; // / 1024
    int32_t mv_sq_div_1000 = (mv * mv) / 1000;                   // V^2 * 1000

    /* NTU*10 = -11204*V^2 + 57423*V - 43529 */
    int32_t ntu_x10 = -((mv_sq_div_1000 * 11204L) / 1000L) + ((mv * 57423L) / 1000L) - 43529L;

    if (ntu_x10 < 0)
        ntu_x10 = 0;
    else if (ntu_x10 > TURBIDITY_MAX_NTU_X10)
        ntu_x10 = TURBIDITY_MAX_NTU_X10;

    return (int16_t)ntu_x10;
}

int16_t turbidity_calibrated_x10(uint16_t raw, int16_t m_x1000, int16_t c_x10)
{
    if (turbidity_is_fault(raw))
        return TURBIDITY_FAULT;

    /* y*10 = (m/1000) * x * 10 + c*10  ->  m_x1000 * x / 100 + c_x10 */
    int32_t y_x10 = ((int32_t)m_x1000 * (int32_t)raw) / 100L + c_x10;

    return clamp_i16(y_x10);
}
