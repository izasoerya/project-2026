# Slave Turbidity — Modbus Register Map

Modbus RTU slave running on an STM8S003K3. It measures turbidity (DFRobot SEN0189-style analog sensor) and water temperature (10k NTC thermistor).

## Serial settings

| Parameter     | Default             |
| ------------- | ------------------- |
| Slave ID      | `1`                 |
| Baud rate     | `9600`              |
| Frame         | 8 data bits, no parity, 1 stop bit (8N1) |
| Flow control  | Automatic (`DE_PIN = MODBUS_DE_PIN_NONE`). Set `DE_PIN` in `src/main.c` for a manual DE/RE pin. |

## Supported function codes

| Code   | Name                     | Notes                                   |
| ------ | ------------------------ | --------------------------------------- |
| `0x03` | Read Holding Registers   | Any range inside addresses `0..10`      |
| `0x06` | Write Single Register    | Only on writable registers (see below)  |

Other function codes return exception `0x01` (Illegal Function).

## Holding registers

All registers are 16-bit. "signed" means the value is an `int16` sent as two's complement (for example `0xFFFF` = `-1`).

| Addr | Name                    | Access | Type     | Scale  | Unit    | Default | Description |
| ---- | ----------------------- | ------ | -------- | ------ | ------- | ------- | ----------- |
| 0    | Turbidity raw           | R      | uint16   | 1      | ADC     | –       | Average of 16 ADC samples, `0..1023` |
| 1    | Turbidity (DFRobot)     | R      | signed   | ×10    | NTU     | –       | DFRobot formula, clamped to `0..3000.0` NTU |
| 2    | Turbidity (calibrated)  | R      | signed   | ×10    | NTU     | –       | User calibration `y = m·x + c`, where `x` = register 0 |
| 3    | Temperature             | R      | signed   | ×10    | °C      | –       | Thermistor reading, e.g. `253` = 25.3 °C |
| 4    | Calibration `m`         | R/W    | signed   | ×1000  | NTU/ADC | `1000`  | Slope, e.g. `-8600` = -8.600 NTU per ADC count |
| 5    | Calibration `c`         | R/W    | signed   | ×10    | NTU     | `0`     | Offset, e.g. `30000` = 3000.0 NTU |
| 6    | *Reserved*              | –      | –        | –      | –       | `0`     | Reads `0`. Writes are rejected |
| 7    | *Reserved*              | –      | –        | –      | –       | `0`     | Reads `0`. Writes are rejected |
| 8    | Restart                 | R/W    | uint16   | –      | –       | `0`     | Write `1` to restart the MCU (after it replies) |
| 9    | Slave ID                | R/W    | uint16   | –      | –       | `1`     | `1..247`. Used from the next request on |
| 10   | Baud rate               | R/W    | enum     | –      | –       | `1`     | `0` = 4800, `1` = 9600, `2` = 19200 |

Registers 0–3 are updated every 250 ms.

### Turbidity formulas

**Register 1 (DFRobot):**

```
V   = raw × 5000 / 1024              (mV, VREF = TURBIDITY_ADC_VREF_MV)
NTU = -1120.4·V² + 5742.3·V - 4352.9  (V in volts)
```

Values below 0 NTU (V above about 4.2 V, clear water) read as `0`. Values above 3000 NTU (V below about 2.5 V) read as `30000`. The firmware uses integer maths only, so it matches the float formula to within about ±0.1 NTU.

**Register 2 (self calibration):**

```
NTU×10 = (m_reg × raw) / 100 + c_reg
```

`m_reg` is the slope ×1000 and `c_reg` is the offset ×10, so this equals `10 × (m·raw + c)`. With the defaults (`m = 1.000`, `c = 0`), register 2 reads `raw × 10`.

**Calibration example:** two reference samples give raw `820` at `0 NTU` and raw `500` at `1000 NTU`.

```
m = (1000 - 0) / (500 - 820) = -3.125   -> write -3125  (0xF3CB) to reg 4
c = 0 - m × 820             = 2562.5   -> write 25625  (0x6419) to reg 5
```

### Fault values

| Register    | Fault value     | Meaning |
| ----------- | --------------- | ------- |
| 1, 2        | `-999` (`0xFC19`)   | Turbidity ADC `< 16` (sensor disconnected or shorted) or `>= 1008` (saturated) |
| 3           | `-999` (`0xFC19`)   | Thermistor ADC out of range |

### Write rules

| Write                               | Response |
| ----------------------------------- | -------- |
| To registers 0–3, 6, 7              | Exception `0x02` (Illegal Data Address) |
| Reg 8 value not `0` or `1`          | Exception `0x03` (Illegal Data Value) |
| Reg 9 value outside `1..247`        | Exception `0x03` |
| Reg 10 value outside `0..2`         | Exception `0x03` |
| Address `>= 11`                     | Exception `0x02` |

Changes to registers 8, 9 and 10 take effect **after** the slave has sent its reply. The reply still uses the old slave ID and baud rate. After a slave ID or baud change, the master must use the new settings.

## Persistence

Registers 4, 5, 9 and 10 are saved in the STM8 data EEPROM (`services/eeprom-service`). They keep their values through a restart (register 8) or a power cycle.

- The slave saves a value after it has replied to the write. Only bytes that changed are written, which limits EEPROM wear (about 300k cycles per byte).
- Register 8 (restart) is not saved. It always reads `0` after boot.
- On first boot (blank EEPROM), or after the firmware's layout version changes, all saved registers are reset to their defaults.
- If a saved value is outside its valid range, the slave uses the default at boot.

EEPROM layout (offset from `0x4000`):

| Offset | Size | Content |
| ------ | ---- | ------- |
| 0      | 1    | Magic `0xA5` |
| 1      | 1    | Layout version |
| 2      | 2    | Calibration `m` (big endian) |
| 4      | 2    | Calibration `c` (big endian) |
| 6      | 1    | Slave ID |
| 7      | 1    | Baud rate index |

**Recovery:** if the slave ID or baud rate is unknown, use the ST-Link (STVP or `stm8flash`) to overwrite the magic byte at `0x4000` with anything other than `0xA5`. On the next boot the slave restores the defaults: ID `1` and 9600 baud.

## Example frames

Read registers 0–5 from slave 1:

```
TX: 01 03 00 00 00 06 C5 C8
RX: 01 03 0C <raw> <dfrobot> <calib> <temp> <m> <c> <crc lo> <crc hi>
```

Change the baud rate to 19200:

```
TX: 01 06 00 0A 00 02 <crc lo> <crc hi>
RX: echo of the request (sent at 9600), then the slave switches to 19200
```
