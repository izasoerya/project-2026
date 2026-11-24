#ifndef MODBUS_MINIMAL_H
#define MODBUS_MINIMAL_H

#include <stdint.h>

#define MODBUS_MAX_REGS 11

/* Pass as dePin to modbus_init when the transceiver handles direction automatically */
#define MODBUS_DE_PIN_NONE 255

/**
 * @brief Calculation bits
 *
 * Req: 1 + 1 + 2 + 2 + 2 = 8 byte
 *      ID + FC + ADD + LEN + CRC
 * Rec: 1 + 1 + 1 + (n * 2) + 2 = 5 + 2n bytes
 *      ID + FC + COUNT + DATA + CRC
 */
#define MODBUS_MAX_FRAME_SIZE (5 + (MODBUS_MAX_REGS * 2))

typedef enum
{
    ILLEGAL_FUNCTION = 0x01,
    ILLEGAL_DATA_ADDRESS = 0x02,
    ILLEGAL_DATA_VALUE = 0x03,
    SLAVE_DEVICE_FAILURE = 0x04,
    UNKNOWN = 0xFF
} ModbusException;
ModbusException _modbus_exception_generator(uint8_t code);

/**
 * @brief Optional hook called before a master write (FC 0x06) is stored.
 * @return 0 to accept, otherwise the ModbusException code sent back to the master.
 */
typedef uint8_t (*modbus_write_validator_t)(uint16_t addr, uint16_t val);

typedef struct
{
    uint16_t holding_regs[MODBUS_MAX_REGS];
    uint8_t slave_id;
    uint8_t de_pin; /* RS485 DE/RE pin, MODBUS_DE_PIN_NONE = automatic flow control */
    modbus_write_validator_t write_validator;
} modbus_t;

/**
 * @brief Initialise the Modbus server.
 * @param dePin RS485 driver-enable pin (DE tied to /RE). Use MODBUS_DE_PIN_NONE (255)
 *              for transceivers with automatic flow control.
 */
void modbus_init(modbus_t *mb, uint8_t slave_id, uint8_t dePin);

/**
 * @brief Switch the RS485 transceiver direction.
 * @param transmit 1 = drive the bus (DE HIGH), 0 = listen (DE LOW).
 * No-op when the server was initialised with MODBUS_DE_PIN_NONE.
 */
void modbus_set_transmit(modbus_t *mb, uint8_t transmit);

/** @brief Register a write validator (NULL = accept every in-range write). */
void modbus_set_write_validator(modbus_t *mb, modbus_write_validator_t validator);

uint16_t modbus_process(modbus_t *mb, const uint8_t *rx_buf, uint16_t rx_len, uint8_t *tx_buf);

uint16_t modbus_read_holding(modbus_t *mb, uint8_t addr);

void modbus_write_holding(modbus_t *mb, uint16_t addr, uint16_t val);

#endif