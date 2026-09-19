#include <Preferences.h>
#if !defined(NVS_MANAGER_H)
#define NVS_MANAGER_H

class NVSManager
{
public:
    static bool ensureRainfall(float defaultValue = 0.0F)
    {
        Preferences prefs;
        if (!prefs.begin("app_config", false))
            return false;

        if (!prefs.isKey("rainfall_counter") && prefs.putFloat("rainfall_counter", defaultValue) == 0)
        {
            prefs.end();
            return false;
        }

        prefs.end();
        return true;
    }

    static bool storeRainfall(float val)
    {
        Preferences prefs;
        if (!prefs.begin("app_config", false))
            return false;

        prefs.putFloat("rainfall_counter", val);
        float counter = prefs.getFloat("rainfall_counter", -1);
        prefs.end();
        return val == counter;
    }

    static bool storeTest(float val)
    {
        Preferences prefs;
        if (!prefs.begin("app_config", false))
            return false;

        prefs.putFloat("test", val);
        float counter = prefs.getFloat("test", -1);
        prefs.end();
        return val == counter;
    }

    static float getRainfall()
    {
        Preferences prefs;
        if (!prefs.begin("app_config", true))
            return -1;

        if (!prefs.isKey("rainfall_counter"))
        {
            prefs.end();
            return -1;
        }

        float counter = prefs.getFloat("rainfall_counter", -1);
        prefs.end();
        return counter;
    }

    static float getTest()
    {
        Preferences prefs;
        if (!prefs.begin("app_config", true))
            return -1;

        if (!prefs.isKey("test"))
        {
            prefs.end();
            return -1;
        }

        float counter = prefs.getFloat("test", -1);
        prefs.end();
        return counter;
    }
};

#endif // NVS_MANAGER_H
