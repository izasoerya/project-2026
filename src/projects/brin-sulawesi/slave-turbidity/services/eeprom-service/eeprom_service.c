#include <Arduino.h>
#include "eeprom_service.h"

#define EEPROM_BASE_ADDR 0x4000

/* Bump the version whenever the layout below changes, so old contents are reset to defaults */
#define EEPROM_MAGIC 0xA5
#define EEPROM_LAYOUT_VERSION 1

/**
 * Layout (offset from EEPROM_BASE_ADDR):
 *   0 : magic
 *   1 : layout version
 *   2 : calib m   (2 bytes, big endian)
 *   4 : calib c   (2 bytes, big endian)
 *   6 : slave id  (1 byte)
 *   7 : baud idx  (1 byte)
 */
#define EEPROM_OFFSET_MAGIC 0
#define EEPROM_OFFSET_VERSION 1

typedef struct
{
    uint8_t offset;
    uint8_t size; // 1 or 2 bytes
    uint16_t default_value;
    uint16_t min_value;
    uint16_t max_value;
} eeprom_param_desc_t;

static const eeprom_param_desc_t PARAMS[EEPROM_PARAM_COUNT] = {
    /* EEPROM_PARAM_CALIB_M    */ {2, 2, 1000, 0x0000, 0xFFFF}, // 1.000, any int16
    /* EEPROM_PARAM_CALIB_C    */ {4, 2, 0, 0x0000, 0xFFFF},    // 0.0, any int16
    /* EEPROM_PARAM_SLAVE_ID   */ {6, 1, 1, 1, 247},
    /* EEPROM_PARAM_BAUD_INDEX */ {7, 1, 1, 0, 2}, // 9600
};

static uint8_t eeprom_read_byte(uint8_t offset)
{
    return *((volatile uint8_t *)(EEPROM_BASE_ADDR + offset));
}

static void eeprom_write_byte(uint8_t offset, uint8_t value)
{
    if (eeprom_read_byte(offset) == value)
        return;

    /* Unlock data EEPROM (MASS keys are reversed compared to program flash) */
    FLASH->DUKR = 0xAE;
    FLASH->DUKR = 0x56;
    while (!(FLASH->IAPSR & FLASH_IAPSR_DUL))
        ;

    *((volatile uint8_t *)(EEPROM_BASE_ADDR + offset)) = value;
    while (!(FLASH->IAPSR & FLASH_IAPSR_EOP))
        ;

    FLASH->IAPSR &= (uint8_t)~FLASH_IAPSR_DUL; // Lock again
}

static uint8_t param_in_range(eeprom_param_t param, uint16_t value)
{
    return value >= PARAMS[param].min_value && value <= PARAMS[param].max_value;
}

void eeprom_service_init(void)
{
    if (eeprom_read_byte(EEPROM_OFFSET_MAGIC) == EEPROM_MAGIC &&
        eeprom_read_byte(EEPROM_OFFSET_VERSION) == EEPROM_LAYOUT_VERSION)
        return;

    for (uint8_t i = 0; i < EEPROM_PARAM_COUNT; i++)
        eeprom_service_write((eeprom_param_t)i, PARAMS[i].default_value);

    /* Header last: a power loss during the defaults write will redo it on next boot */
    eeprom_write_byte(EEPROM_OFFSET_VERSION, EEPROM_LAYOUT_VERSION);
    eeprom_write_byte(EEPROM_OFFSET_MAGIC, EEPROM_MAGIC);
}

uint16_t eeprom_service_read(eeprom_param_t param)
{
    if (param >= EEPROM_PARAM_COUNT)
        return 0;

    const eeprom_param_desc_t *desc = &PARAMS[param];
    uint16_t value = eeprom_read_byte(desc->offset);
    if (desc->size == 2)
        value = (value << 8) | eeprom_read_byte(desc->offset + 1);

    return param_in_range(param, value) ? value : desc->default_value;
}

void eeprom_service_write(eeprom_param_t param, uint16_t value)
{
    if (param >= EEPROM_PARAM_COUNT || !param_in_range(param, value))
        return;

    const eeprom_param_desc_t *desc = &PARAMS[param];
    if (desc->size == 2)
    {
        eeprom_write_byte(desc->offset, value >> 8);
        eeprom_write_byte(desc->offset + 1, value & 0xFF);
    }
    else
        eeprom_write_byte(desc->offset, (uint8_t)value);
}
