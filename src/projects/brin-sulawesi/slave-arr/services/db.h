#if !defined(DB_H)
#define DB_H

#include <Arduino.h>
#include <Preferences.h>
#include "../consts/global_config.h"
#include "../models/task_context.h" // ModbusRegister::baudToCode

class DB
{
private:
    static constexpr const char *NAMESPACE = "app_config";
    static constexpr const char *KEY_SLAVE_ID = "slave_id";
    static constexpr const char *KEY_BAUD_RATE = "baud_rate";
    static constexpr const char *KEY_VERSION = "version";

    static bool open(Preferences &prefs, bool readOnly)
    {
        if (!prefs.begin(NAMESPACE, readOnly))
        {
            // Read-only open fails on first boot when the namespace doesn't exist yet;
            // callers fall back to defaults, so only report write failures.
            if (!readOnly)
                Serial.println("[ERROR] DB: failed to open NVS namespace");
            return false;
        }
        return true;
    }

public:
    // ---------- Modbus slave id (1..247) ----------
    static bool saveSlaveId(uint8_t slaveId)
    {
        Preferences prefs;
        if (!open(prefs, false))
            return false;
        const bool ok = prefs.putUChar(KEY_SLAVE_ID, slaveId) == sizeof(uint8_t);
        prefs.end();
        return ok;
    }

    // Returns the stored id, or defaultId if missing or out of range
    static uint8_t getSlaveId(uint8_t defaultId = GlobalConfig::SLAVE_ID)
    {
        Preferences prefs;
        if (!open(prefs, true))
            return defaultId;
        const uint8_t id = prefs.getUChar(KEY_SLAVE_ID, defaultId);
        prefs.end();
        return (id >= 1 && id <= 247) ? id : defaultId;
    }

    // ---------- Modbus baud rate ----------
    static bool saveBaudRate(uint32_t baudRate)
    {
        Preferences prefs;
        if (!open(prefs, false))
            return false;
        const bool ok = prefs.putUInt(KEY_BAUD_RATE, baudRate) == sizeof(uint32_t);
        prefs.end();
        return ok;
    }

    // Returns the stored baud, or defaultBaud if missing or not in BAUD_TABLE
    static uint32_t getBaudRate(uint32_t defaultBaud = GlobalConfig::MODBUS_BAUD_RATE)
    {
        Preferences prefs;
        if (!open(prefs, true))
            return defaultBaud;
        const uint32_t baud = prefs.getUInt(KEY_BAUD_RATE, defaultBaud);
        prefs.end();
        return ModbusRegister::baudToCode(baud) != ModbusRegister::BAUD_INVALID ? baud : defaultBaud;
    }

    // ---------- OTA version ----------
    static bool saveVersion(const char *version)
    {
        if (version == nullptr)
            return false;
        Preferences prefs;
        if (!open(prefs, false))
            return false;
        const bool ok = prefs.putString(KEY_VERSION, version) == strlen(version);
        prefs.end();
        return ok;
    }

    static String getVersion(const char *defaultVersion = "")
    {
        Preferences prefs;
        if (!open(prefs, true))
            return String(defaultVersion);
        const String version = prefs.getString(KEY_VERSION, defaultVersion);
        prefs.end();
        return version;
    }
};

#endif // DB_H
