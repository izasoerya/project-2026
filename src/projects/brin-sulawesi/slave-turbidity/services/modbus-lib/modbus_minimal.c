#include <Arduino.h>
#include "modbus_minimal.h"

/* Standard Modbus CRC-16 */
static uint16_t modbus_crc(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

void modbus_init(modbus_t *mb, uint8_t slave_id, uint8_t dePin)
{
    mb->slave_id = slave_id;
    mb->de_pin = dePin;
    mb->write_validator = 0;
    for (int i = 0; i < MODBUS_MAX_REGS; i++)
        mb->holding_regs[i] = 0;

    if (mb->de_pin != MODBUS_DE_PIN_NONE)
    {
        pinMode(mb->de_pin, OUTPUT);
        digitalWrite(mb->de_pin, LOW); /* Default to receive mode */
    }
}

void modbus_set_transmit(modbus_t *mb, uint8_t transmit)
{
    if (mb->de_pin == MODBUS_DE_PIN_NONE)
        return;

    digitalWrite(mb->de_pin, transmit ? HIGH : LOW);
}

void modbus_set_write_validator(modbus_t *mb, modbus_write_validator_t validator)
{
    mb->write_validator = validator;
}

void modbus_write_holding(modbus_t *mb, uint16_t addr, uint16_t val)
{
    if (addr < MODBUS_MAX_REGS)
    {
        mb->holding_regs[addr] = val;
    }
}

uint16_t modbus_read_holding(modbus_t *mb, uint8_t addr)
{
    if (addr >= MODBUS_MAX_REGS)
        return 0;

    return mb->holding_regs[addr];
}

uint16_t modbus_process(modbus_t *mb, const uint8_t *rx_buf, uint16_t rx_len, uint8_t *tx_buf)
{
    if (rx_len != 8 || rx_buf[0] != mb->slave_id)
        return 0;

    uint16_t crc_expected = rx_buf[6] | ((uint16_t)rx_buf[7] << 8);
    if (modbus_crc(rx_buf, 6) != crc_expected)
        return 0;

    uint8_t function = rx_buf[1];

    if (function == 0x03)
    {
        uint16_t start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
        uint16_t reg_count = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];
        if (reg_count < 1 || reg_count > 125)
        {
            ModbusException exc = _modbus_exception_generator(ILLEGAL_DATA_VALUE);

            tx_buf[0] = mb->slave_id;
            tx_buf[1] = function | 0x80;
            tx_buf[2] = exc;
            uint16_t crc = modbus_crc(tx_buf, 3);
            tx_buf[3] = crc & 0xFF;
            tx_buf[4] = crc >> 8;

            return 5;
        }

        if (start_addr >= MODBUS_MAX_REGS || reg_count > (MODBUS_MAX_REGS - start_addr))
        {
            ModbusException exc = _modbus_exception_generator(ILLEGAL_DATA_ADDRESS);

            tx_buf[0] = mb->slave_id;
            tx_buf[1] = function | 0x80;
            tx_buf[2] = exc;
            uint16_t crc = modbus_crc(tx_buf, 3);
            tx_buf[3] = crc & 0xFF;
            tx_buf[4] = crc >> 8;

            return 5;
        }

        tx_buf[0] = mb->slave_id;
        tx_buf[1] = 0x03;
        tx_buf[2] = reg_count * 2;
        for (uint8_t read_regs_iter = 0; read_regs_iter < reg_count; read_regs_iter++)
        {
            uint16_t value = mb->holding_regs[start_addr + read_regs_iter];
            tx_buf[3 + read_regs_iter * 2] = value >> 8;
            tx_buf[4 + read_regs_iter * 2] = value & 0xFF;
        }
        uint16_t response_len = 3 + (reg_count * 2);
        uint16_t crc = modbus_crc(tx_buf, response_len);
        tx_buf[response_len] = crc & 0xFF;
        tx_buf[response_len + 1] = crc >> 8;

        return response_len + 2;
    }

    else if (function == 0x06)
    {
        uint16_t target_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];

        if (target_addr >= MODBUS_MAX_REGS)
        {
            ModbusException exc = _modbus_exception_generator(ILLEGAL_DATA_ADDRESS);
            tx_buf[0] = mb->slave_id;
            tx_buf[1] = function | 0x80;
            tx_buf[2] = exc;
            uint16_t crc = modbus_crc(tx_buf, 3);
            tx_buf[3] = crc & 0xFF;
            tx_buf[4] = crc >> 8;

            return 5;
        }

        uint16_t value = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];

        uint8_t reject = mb->write_validator ? mb->write_validator(target_addr, value) : 0;
        if (reject)
        {
            tx_buf[0] = mb->slave_id;
            tx_buf[1] = function | 0x80;
            tx_buf[2] = _modbus_exception_generator(reject);
            uint16_t reject_crc = modbus_crc(tx_buf, 3);
            tx_buf[3] = reject_crc & 0xFF;
            tx_buf[4] = reject_crc >> 8;

            return 5;
        }

        mb->holding_regs[target_addr] = value;
        for (uint8_t write_reg_iter = 0; write_reg_iter < 8; write_reg_iter++)
            tx_buf[write_reg_iter] = rx_buf[write_reg_iter];

        return 8;
    }

    else
    {
        ModbusException exc = _modbus_exception_generator(ILLEGAL_FUNCTION);

        tx_buf[0] = mb->slave_id;
        tx_buf[1] = function | 0x80;
        tx_buf[2] = exc;
        uint16_t crc = modbus_crc(tx_buf, 3);
        tx_buf[3] = crc & 0xFF;
        tx_buf[4] = crc >> 8;

        return 5;
    }
}

ModbusException _modbus_exception_generator(uint8_t code)
{
    switch (code)
    {
    case 0x01:
        return ILLEGAL_FUNCTION;

    case 0x02:
        return ILLEGAL_DATA_ADDRESS;

    case 0x03:
        return ILLEGAL_DATA_VALUE;

    case 0x04:
        return SLAVE_DEVICE_FAILURE;

    default:
        return UNKNOWN;
    }
}