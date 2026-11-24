#include <Arduino.h>
#include "services/modbus-lib/modbus_minimal.h"
#include "thermistor.h"
#include "df_turbidity.h"

#define SLAVE_ID 1

static modbus_t modbus_server;
static uint8_t modbus_rx[MODBUS_MAX_FRAME_SIZE];
static uint8_t modbus_tx[MODBUS_MAX_FRAME_SIZE];
static uint8_t modbus_rx_len = 0;
static uint32_t modbus_last_byte_ms = 0;
static uint64_t counter = 0;

void setup(void)
{
	Serial_begin(9600);

	modbus_init(&modbus_server, SLAVE_ID);
	modbus_write_holding(&modbus_server, 1, 1); // m
	modbus_write_holding(&modbus_server, 2, 0); // c
}

void modbus_server_task(void)
{
	uint32_t now = millis();

	/**
	 * @brief Calculation of frame timeout in modbus
	 * Modbus protocol have SOP that messages have silent interval of 3.5 ms
	 * Since the payload is 1 start bit + 8 data bits + 0 parity bit + 1 stop bit = 11 bits
	 * It will require 10x1/9600 + 3.5 ms = 4.51 ms (where 5 ms is safe since millis +- 1ms)
	 */
	const uint32_t INTER_FRAME_MS = 5;

	while (Serial_available() > 0)
	{
		if (modbus_rx_len == 0)
			modbus_last_byte_ms = now;
		else if ((now - modbus_last_byte_ms) > INTER_FRAME_MS) // Payload complete, time to reset
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
	}

	/* Process complete frame upon timeout */
	if (modbus_rx_len > 0 && (now - modbus_last_byte_ms) > INTER_FRAME_MS)
	{
		uint16_t tx_len = modbus_process(&modbus_server, modbus_rx, modbus_rx_len, modbus_tx);
		if (tx_len > 0)
		{
			for (uint8_t i = 0; i < tx_len; i++)
				Serial_write(modbus_tx[i]);

			Serial_flush(); /* Block until hardware UART shift register is completely empty */
		}
		modbus_rx_len = 0;
		counter++;
	}
}

void loop(void)
{
	modbus_server_task();

	static uint32_t last_adc_ms = 0;
	if (millis() - last_adc_ms >= 250)
	{
		float turbidity_multiplied_by_10 = read_turbidity();
		int16_t temperature_multiplied_by_10 = read_thermistor_temp();

		modbus_write_holding(&modbus_server, 0, turbidity_multiplied_by_10);
		modbus_write_holding(&modbus_server, 4, temperature_multiplied_by_10);
		modbus_write_holding(&modbus_server, 5, counter);

		last_adc_ms = millis();
	}
}