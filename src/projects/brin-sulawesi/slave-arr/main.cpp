#include <Arduino.h>
#include <ModbusRTUSlave.h>
#include <time.h>
#include <sys/time.h>

#include "transmitter/configs/wifi_module.h"
#include "consts/global_config.h"
#include "models/task_context.h"
#include "models/datastore.h"
#include "services/db.h"
#include "utils/time_utils.h"

static void IRAM_ATTR onTipISR(void *arg);
static void handlerTipWindows(SensorContext &sensorCtx);
static void handlerWrites(uint16_t *regs, ModbusRTUSlave &mb, ModbusContext &mbCtx);
static void handlerReads(uint16_t *holdingRegisters,
                         SensorContext &sensorCtx,
                         NetworkingContext &networkCtx,
                         ModbusContext &mbCtx);

void setup()
{
    Serial.begin(115200);
    setenv("TZ", GlobalConfig::TIMEZONE, 1); // localtime_r() -> local 00:00 for the daily reset
    tzset();

    static SensorContext sensorCtx;
    static ModbusContext mbCtx;

    mbCtx.slaveId = DB::getSlaveId(mbCtx.slaveId);
    mbCtx.baudRate = DB::getBaudRate(mbCtx.baudRate);

    static WiFiModule inet(
        GlobalConfig::ssid, GlobalConfig::password, GlobalConfig::hostname,
        wifi_power_t::WIFI_POWER_8_5dBm);
    static MQTTWebsocketSecure mqtt(GlobalConfig::BROKER_MQTT, GlobalConfig::PORT_MQTT);
    static NetworkingContext networkCtx(inet, mqtt);

    GlobalConfig::mbSerial.begin(mbCtx.baudRate, SERIAL_8N1, ModbusContext::PIN_RX, ModbusContext::PIN_TX);
    static ModbusRTUSlave mb(GlobalConfig::mbSerial);
    static uint16_t holdingRegisters[ModbusRegister::COUNT] = {};
    mb.configureHoldingRegisters(holdingRegisters, ModbusRegister::COUNT);
    mb.begin(mbCtx.slaveId, mbCtx.baudRate);
    Serial.printf("[INFO] Init modbus slave id %u @ %lu baud\n",
                  mbCtx.slaveId, (unsigned long)mbCtx.baudRate);

    // Tipping bucket reed switch to GND
    sensorCtx.startupMs = TimeUtils::uptimeMs(); // fallback window origin while the clock is not set
    handlerTipWindows(sensorCtx);                // initialise window keys
    pinMode(SensorContext::PIN_INT, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(SensorContext::PIN_INT), onTipISR, &sensorCtx, FALLING);

    handlerReads(holdingRegisters, sensorCtx, networkCtx, mbCtx);

    while (1)
    {
        if (mb.poll())
            handlerWrites(holdingRegisters, mb, mbCtx);

        static uint32_t lastRefresh = millis();
        if (millis() - lastRefresh >= 1000)
        {
            lastRefresh = millis();
            handlerTipWindows(sensorCtx);
            handlerReads(holdingRegisters, sensorCtx, networkCtx, mbCtx);
        }
    }
}

void loop() {}

// Count one tip per debounced falling edge. Runs from IRAM: no Serial, delay or alloc.
static void IRAM_ATTR onTipISR(void *arg)
{
    SensorContext *ctx = static_cast<SensorContext *>(arg);
    const uint32_t now = millis();

    portENTER_CRITICAL_ISR(&ctx->mux);
    if (now - ctx->lastTipMs >= SensorContext::DEBOUNCE_MS)
    {
        ctx->lastTipMs = now;
        ctx->tipHour++;
        ctx->tipHalfDay++;
        ctx->tipDay++;
    }
    portEXIT_CRITICAL_ISR(&ctx->mux);
}

// Zero each counter when its window changes.
static void handlerTipWindows(SensorContext &sensorCtx)
{
    struct tm timeinfo;
    const time_t now = time(nullptr);
    static bool fromClock = false;
    if (now > static_cast<time_t>(GlobalConfig::MIN_VALID_UNIX))
    {
        localtime_r(&now, &timeinfo);
        fromClock = true;
    }

    int32_t dayKey, halfDayKey, hourKey;
    if (fromClock)
    {
        dayKey = (timeinfo.tm_year + 1900) * 1000 + timeinfo.tm_yday; // changes at local 00:00
        halfDayKey = dayKey * 2 + timeinfo.tm_hour / 12;
        hourKey = dayKey * 24 + timeinfo.tm_hour;
    }
    else
    {
        const uint64_t elapsed = TimeUtils::uptimeMs() - sensorCtx.startupMs;
        dayKey = static_cast<int32_t>(elapsed / SensorContext::DAY_MS);
        halfDayKey = static_cast<int32_t>(elapsed / SensorContext::HALF_DAY_MS);
        hourKey = static_cast<int32_t>(elapsed / SensorContext::HOUR_MS);
    }

    const bool adoptOnly = sensorCtx.dayKey < 0 || fromClock != sensorCtx.keysFromClock;
    if (!adoptOnly)
    {
        portENTER_CRITICAL(&sensorCtx.mux);
        if (dayKey != sensorCtx.dayKey)
            sensorCtx.tipDay = 0;
        if (halfDayKey != sensorCtx.halfDayKey)
            sensorCtx.tipHalfDay = 0;
        if (hourKey != sensorCtx.hourKey)
            sensorCtx.tipHour = 0;
        portEXIT_CRITICAL(&sensorCtx.mux);
    }

    sensorCtx.keysFromClock = fromClock;
    sensorCtx.dayKey = dayKey;
    sensorCtx.halfDayKey = halfDayKey;
    sensorCtx.hourKey = hourKey;
}

// Update modbus registers value
static void handlerReads(
    uint16_t *holdingRegisters,
    SensorContext &sensorCtx,
    NetworkingContext &networkCtx,
    ModbusContext &mbCtx)
{
    portENTER_CRITICAL(&sensorCtx.mux);
    const SensorObject sensor{
        .tippingDay = sensorCtx.tipDay,
        .tippingHalfDay = sensorCtx.tipHalfDay,
        .tippingHour = sensorCtx.tipHour,
    };
    portEXIT_CRITICAL(&sensorCtx.mux);

    // 0 tells the master the clock is not set
    const time_t now = time(nullptr);
    mbCtx.unixTime = now >= static_cast<time_t>(GlobalConfig::MIN_VALID_UNIX) ? static_cast<uint32_t>(now) : 0;

    ModbusSensorAgregate agregate(sensor);
    agregate.setUnixTime(mbCtx.unixTime);
    agregate.setSlaveId(mbCtx.slaveId);
    agregate.setBaud(mbCtx.baudRate);
    agregate.setRSSI(networkCtx.wifi.getRssi());
    agregate.toArray(holdingRegisters); // Apply agregate data to holding regs
}

// Apply writes from the master to the (rw) registers. Invalid values are reverted.
static void handlerWrites(uint16_t *regs, ModbusRTUSlave &mb, ModbusContext &mbCtx)
{
    using namespace ModbusRegister;

    const uint32_t newUnix = BitMask::decodeU32(&regs[UNIX_TIME]);
    if (newUnix != mbCtx.unixTime)
    {
        if (newUnix >= GlobalConfig::MIN_VALID_UNIX)
        {
            const timeval tv = {.tv_sec = static_cast<time_t>(newUnix), .tv_usec = 0};
            settimeofday(&tv, nullptr);
            mbCtx.unixTime = newUnix;
            Serial.printf("[INFO] Clock set to %lu\n", (unsigned long)newUnix);
        }
        else
        {
            Serial.printf("[ERROR] Invalid unix time: %lu\n", (unsigned long)newUnix);
            BitMask::encodeU32(mbCtx.unixTime, &regs[UNIX_TIME]);
        }
    }

    // Slave id (valid modbus unit id: 1..247)
    if (regs[SLAVE_ID] != mbCtx.slaveId)
    {
        const uint16_t newId = regs[SLAVE_ID];
        if (newId >= 1 && newId <= 247)
        {
            Serial.printf("[INFO] Modbus slave id %u -> %u\n", mbCtx.slaveId, newId);
            mbCtx.slaveId = newId;
            if (!DB::saveSlaveId(mbCtx.slaveId))
                Serial.println("[ERROR] Failed to save slave id");
            mb.begin(mbCtx.slaveId, mbCtx.baudRate);
        }
        else
        {
            Serial.printf("[ERROR] Invalid slave id: %u\n", newId);
            regs[SLAVE_ID] = mbCtx.slaveId;
        }
    }

    // Baud code (0: 4800, 1: 9600, 2: 19200)
    if (regs[BAUD_RATE] != baudToCode(mbCtx.baudRate))
    {
        bool matched = false;
        for (uint16_t code = 0; code < BAUD_COUNT; code++)
        {
            if (regs[BAUD_RATE] != code)
                continue;

            Serial.printf("[INFO] Modbus baud %lu -> %lu\n",
                          (unsigned long)mbCtx.baudRate, (unsigned long)BAUD_TABLE[code]);
            mbCtx.baudRate = BAUD_TABLE[code];
            if (!DB::saveBaudRate(mbCtx.baudRate))
                Serial.println("[ERROR] Failed to save baud rate");
            GlobalConfig::mbSerial.flush(); // let the ack go out at the old baud first
            GlobalConfig::mbSerial.updateBaudRate(mbCtx.baudRate);
            mb.begin(mbCtx.slaveId, mbCtx.baudRate); // recompute frame timing
            matched = true;
            break;
        }

        if (!matched)
        {
            Serial.printf("[ERROR] Invalid baud code: %u\n", regs[BAUD_RATE]);
            regs[BAUD_RATE] = baudToCode(mbCtx.baudRate);
        }
    }

    // Restart
    if (regs[RESTART] == 1)
    {
        Serial.println("[INFO] Modbus restart requested");
        GlobalConfig::mbSerial.flush(); // let the ack go out before rebooting
        delay(100);
        esp_restart();
    }
    else if (regs[RESTART] != 0)
        regs[RESTART] = 0;
}
