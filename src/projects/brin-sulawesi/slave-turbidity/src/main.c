#include <Arduino.h>
#include "services/modbus-lib/modbus_minimal.h"
#include "services/eeprom-service/eeprom_service.h"
#include "thermistor.h"
#include "df_turbidity.h"

#define DE_PIN MODBUS_DE_PIN_NONE // DE/RE flow control

/* Holding register map, see docs/modbus-registers.md */
#define REG_TURBIDITY_RAW 0     // R  : averaged ADC (0..1023)
#define REG_TURBIDITY_DFROBOT 1 // R  : NTU x10, DFRobot curve
#define REG_TURBIDITY_CALIB 2   // R  : NTU x10, y = m*x + c
#define REG_TEMPERATURE 3       // R  : degC x10
#define REG_CALIB_M 4           // RW : slope x1000 (signed)
#define REG_CALIB_C 5           // RW : offset NTU x10 (signed)
#define REG_RESTART 8           // RW : write 1 to restart MCU
#define REG_SLAVE_ID 9          // RW : 1..247
#define REG_BAUD_RATE 10        // RW : 0=4800, 1=9600, 2=19200

static modbus_t modbus_server;
static uint8_t modbus_rx[MODBUS_MAX_FRAME_SIZE];
static uint8_t modbus_tx[MODBUS_MAX_FRAME_SIZE];
static uint8_t modbus_rx_len = 0;
static uint32_t modbus_last_byte_ms = 0;

/**
 * @brief Modbus RTU inter-frame silence = 3.5 character times (8N1 -> 10 bits/char)
 * 4800  : 3.5 * 10 / 4800  = 7.29 ms -> 8 ms
 * 9600  : 3.5 * 10 / 9600  = 3.65 ms -> 5 ms
 * 19200 : 3.5 * 10 / 19200 = 1.82 ms -> 3 ms
 * Rounded up with +1 ms margin since millis() resolution is +-1 ms.
 */
static const uint32_t BAUD_TABLE[3] = {4800, 9600, 19200};
static const uint8_t INTER_FRAME_MS_TABLE[3] = {8, 5, 3};
static uint8_t baud_index;

static void software_reset(void)
{
	WWDG->CR = 0x80; // Enable WWDG with T6 = 0 -> immediate reset
	while (1)
		;
}

static uint8_t modbus_validate_write(uint16_t addr, uint16_t val)
{
	switch (addr)
	{
	case REG_CALIB_M:
	case REG_CALIB_C:
		return 0;

	case REG_RESTART:
		return val <= 1 ? 0 : ILLEGAL_DATA_VALUE;

	case REG_SLAVE_ID:
		return (val >= 1 && val <= 247) ? 0 : ILLEGAL_DATA_VALUE;

	case REG_BAUD_RATE:
		return val <= 2 ? 0 : ILLEGAL_DATA_VALUE;

	default: // Measurement registers are read-only, 6..7 reserved
		return ILLEGAL_DATA_ADDRESS;
	}
}

/* Apply configuration writes only after the response has been sent with the old settings */
static void apply_pending_config(void)
{
	/* Persist first so a restart request keeps the latest configuration (unchanged bytes are skipped) */
	eeprom_service_write(EEPROM_PARAM_CALIB_M, modbus_read_holding(&modbus_server, REG_CALIB_M));
	eeprom_service_write(EEPROM_PARAM_CALIB_C, modbus_read_holding(&modbus_server, REG_CALIB_C));
	eeprom_service_write(EEPROM_PARAM_SLAVE_ID, modbus_read_holding(&modbus_server, REG_SLAVE_ID));
	eeprom_service_write(EEPROM_PARAM_BAUD_INDEX, modbus_read_holding(&modbus_server, REG_BAUD_RATE));

	if (modbus_read_holding(&modbus_server, REG_RESTART) == 1)
		software_reset();

	modbus_server.slave_id = (uint8_t)modbus_read_holding(&modbus_server, REG_SLAVE_ID);

	uint8_t new_baud_index = (uint8_t)modbus_read_holding(&modbus_server, REG_BAUD_RATE);
	if (new_baud_index != baud_index)
	{
		baud_index = new_baud_index;
		Serial_end();
		Serial_begin(BAUD_TABLE[baud_index]);
	}
}

void setup(void)
{
	eeprom_service_init();

	uint8_t slave_id = (uint8_t)eeprom_service_read(EEPROM_PARAM_SLAVE_ID);
	baud_index = (uint8_t)eeprom_service_read(EEPROM_PARAM_BAUD_INDEX);

	Serial_begin(BAUD_TABLE[baud_index]);

	modbus_init(&modbus_server, slave_id, DE_PIN);
	modbus_set_write_validator(&modbus_server, modbus_validate_write);

	modbus_write_holding(&modbus_server, REG_CALIB_M, eeprom_service_read(EEPROM_PARAM_CALIB_M));
	modbus_write_holding(&modbus_server, REG_CALIB_C, eeprom_service_read(EEPROM_PARAM_CALIB_C));
	modbus_write_holding(&modbus_server, REG_RESTART, 0);
	modbus_write_holding(&modbus_server, REG_SLAVE_ID, slave_id);
	modbus_write_holding(&modbus_server, REG_BAUD_RATE, baud_index);
}

void modbus_server_task(void)
{
	uint32_t now = millis();
	const uint32_t inter_frame_ms = INTER_FRAME_MS_TABLE[baud_index];

	while (Serial_available() > 0)
	{
		if (modbus_rx_len == 0)
			modbus_last_byte_ms = now;
		else if ((now - modbus_last_byte_ms) > inter_frame_ms) // Payload complete, time to reset
		{
			modbus_rx_len = 0;
			modbus_last_byte_ms = now;
		}

		/* Prevent buffer overflow by dropping excess bytes */
		if (modbus_rx_len < MODBUS_MAX_FRAME_SIZE)
		{
			modbus_rx[modbus_rx_len++] = Serial_read();
			modbus_last_byte_ms = now;
		}
		else
			Serial_read();
	}

	/* Process complete frame upon timeout */
	if (modbus_rx_len > 0 && (now - modbus_last_byte_ms) > inter_frame_ms)
	{
		uint16_t tx_len = modbus_process(&modbus_server, modbus_rx, modbus_rx_len, modbus_tx);
		if (tx_len > 0)
		{
			modbus_set_transmit(&modbus_server, 1);
			for (uint8_t i = 0; i < tx_len; i++)
				Serial_write(modbus_tx[i]);

			Serial_flush(); /* Block until hardware UART shift register is completely empty */
			modbus_set_transmit(&modbus_server, 0);

			apply_pending_config();
		}
		modbus_rx_len = 0;
	}
}

void loop(void)
{
	modbus_server_task();

	static uint32_t last_adc_ms = 0;
	if (millis() - last_adc_ms >= 250)
	{
		uint16_t raw = read_turbidity_raw();
		int16_t m = (int16_t)modbus_read_holding(&modbus_server, REG_CALIB_M);
		int16_t c = (int16_t)modbus_read_holding(&modbus_server, REG_CALIB_C);

		modbus_write_holding(&modbus_server, REG_TURBIDITY_RAW, raw);
		modbus_write_holding(&modbus_server, REG_TURBIDITY_DFROBOT, (uint16_t)turbidity_dfrobot_x10(raw));
		modbus_write_holding(&modbus_server, REG_TURBIDITY_CALIB, (uint16_t)turbidity_calibrated_x10(raw, m, c));
		modbus_write_holding(&modbus_server, REG_TEMPERATURE, (uint16_t)read_thermistor_temp());

		last_adc_ms = millis();
	}
}
