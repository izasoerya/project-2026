#ifndef MODBUS_MINIMAL_H
#define MODBUS_MINIMAL_H

#include <stdint.h>

#define MODBUS_MAX_REGS 8

/**
 * @brief Calculation bits
 *
 * Req: 1 + 1 + 2 + 2 + 2 = 8 byte
 *      ID + FC + ADD + LEN + CRC
 * Rec: 1 + 1 + 1 + (n * 2) + 2 = 5 + 2n bytes
 *      ID + FC + COUNT + DATA + CRC
 */
#define MODBUS_MAX_FRAME_SIZE 5 + (MODBUS_MAX_REGS * 2)

typedef enum
{
    ILLEGAL_FUNCTION = 0x01,
    ILLEGAL_DATA_ADDRESS = 0x02,
    ILLEGAL_DATA_VALUE = 0x03,
    SLAVE_DEVICE_FAILURE = 0x04,
    UNKNOWN = 0xFF
} ModbusException;
ModbusException _modbus_exception_generator(uint8_t code);

typedef struct
{
    uint16_t holding_regs[MODBUS_MAX_REGS];
    uint8_t slave_id;
} modbus_t;

void modbus_init(modbus_t *mb, uint8_t slave_id);

uint16_t modbus_process(modbus_t *mb, const uint8_t *rx_buf, uint16_t rx_len, uint8_t *tx_buf);

uint16_t modbus_read_holding(modbus_t *mb, uint8_t addr);

void modbus_write_holding(modbus_t *mb, uint16_t addr, uint16_t val);

#endif