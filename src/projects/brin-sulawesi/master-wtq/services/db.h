#if !defined(DB_H)
#define DB_H

#include <Arduino.h>
#include <Preferences.h>
#include "../consts/global_config.h"
#include "../models/profile.h"

class DB
{
private:
    static constexpr const char *NAMESPACE = "app_config";
    static constexpr const char *KEY_INTERVAL = "interval_ms";
    static constexpr const char *KEY_PROFILE_BOTTOM = "prof_bottom";
    static constexpr const char *KEY_PROFILE_TOP = "prof_top";
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
    // ---------- Interval (milliseconds) ----------
    static bool saveInterval(uint32_t ms)
    {
        Preferences prefs;
        if (!open(prefs, false))
            return false;
        const bool ok = prefs.putUInt(KEY_INTERVAL, ms) == sizeof(uint32_t);
        prefs.end();
        return ok;
    }

    static uint32_t getInterval(uint32_t defaultMs = GlobalConfig::DELAY_TX)
    {
        Preferences prefs;
        if (!open(prefs, true))
            return defaultMs;
        const uint32_t ms = prefs.getUInt(KEY_INTERVAL, defaultMs);
        prefs.end();
        return ms;
    }

    // ---------- Battery profile ----------
    static bool saveProfile(float bottom, float top)
    {
        Preferences prefs;
        if (!open(prefs, false))
            return false;
        const bool ok = prefs.putFloat(KEY_PROFILE_BOTTOM, bottom) == sizeof(float) &&
                        prefs.putFloat(KEY_PROFILE_TOP, top) == sizeof(float);
        prefs.end();
        return ok;
    }

    static bool saveProfile(Profile &profile)
    {
        return saveProfile(profile.getBottomSet(), profile.getTopSet());
    }

    static Profile getProfile(float defaultBottom = GlobalConfig::BATTERY_DANGER,
                              float defaultTop = GlobalConfig::BATTERY_WARNING)
    {
        Preferences prefs;
        if (!open(prefs, true))
            return Profile(defaultBottom, defaultTop);
        const float bottom = prefs.getFloat(KEY_PROFILE_BOTTOM, defaultBottom);
        const float top = prefs.getFloat(KEY_PROFILE_TOP, defaultTop);
        prefs.end();
        return Profile(bottom, top);
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
