#ifndef EEPROM_SERVICE_H
#define EEPROM_SERVICE_H

#include <stdint.h>

/**
 * @brief Persistent configuration stored in the STM8 data EEPROM (128 bytes on STM8S003).
 *
 * The service owns the layout (address, size, default and valid range of every
 * parameter). Callers only deal with parameter ids and values.
 */
typedef enum
{
    EEPROM_PARAM_CALIB_M = 0, // int16, slope x1000
    EEPROM_PARAM_CALIB_C,     // int16, offset NTU x10
    EEPROM_PARAM_SLAVE_ID,    // uint8, 1..247
    EEPROM_PARAM_BAUD_INDEX,  // uint8, 0..2
    EEPROM_PARAM_COUNT
} eeprom_param_t;

/**
 * @brief Check the stored layout. If the EEPROM is blank or holds an older
 * layout version, all parameters are rewritten with their defaults.
 */
void eeprom_service_init(void);

/**
 * @brief Read a parameter. Returns its default if the stored value is outside the valid range.
 */
uint16_t eeprom_service_read(eeprom_param_t param);

/**
 * @brief Store a parameter. Only bytes that differ are written (saves EEPROM wear).
 * Values outside the valid range are ignored.
 */
void eeprom_service_write(eeprom_param_t param, uint16_t value);

#endif
