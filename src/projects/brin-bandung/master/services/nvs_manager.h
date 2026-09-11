#include <Preferences.h>
#if !defined(NVS_MANAGER_H)
#define NVS_MANAGER_H

class NVSManager
{
public:
    static bool storeRainfall(float val)
    {
        Preferences prefs;
        prefs.begin("app_config", false);
        prefs.putFloat("rainfall_counter", val);
        float counter = prefs.getFloat("rainfall_counter", -1);
        prefs.end();
        if (val == counter)
            return true;
        return false;
    }

    static float getRainfall()
    {
        Preferences prefs;
        prefs.begin("app_config", true);
        float counter = prefs.getFloat("rainfall_counter", -1);
        return counter;
    }
};

#endif // NVS_MANAGER_H
