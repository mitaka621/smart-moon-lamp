#pragma once

#include <Preferences.h>

class RelayStateStorage
{
public:
    bool Begin()
    {
        return _preferences.begin(NAMESPACE_NAME, false);
    }

    bool Load(bool defaultIsOn)
    {
        return _preferences.getBool(KEY_IS_ON, defaultIsOn);
    }

    bool Save(bool isOn)
    {
        return _preferences.putBool(KEY_IS_ON, isOn) > 0;
    }

private:
    static constexpr const char* NAMESPACE_NAME = "moonlamp";
    static constexpr const char* KEY_IS_ON = "relayIsOn";

    Preferences _preferences;
};
