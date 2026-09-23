#pragma once

#include <Arduino.h>

class Relay
{
public:
    Relay(uint8_t pin, bool activeLow)
        : _pin(pin), _activeLow(activeLow), _isOn(false)
    {
    }

    void Begin(bool isOn)
    {
        pinMode(_pin, OUTPUT);
        Set(isOn);
    }

    void Set(bool isOn)
    {
        _isOn = isOn;
        digitalWrite(_pin, LevelFor(isOn));
    }

    void Toggle()
    {
        Set(!_isOn);
    }

    bool IsOn() const
    {
        return _isOn;
    }

private:
    uint8_t LevelFor(bool isOn) const
    {
        return isOn != _activeLow ? HIGH : LOW;
    }

    uint8_t _pin;
    bool _activeLow;
    bool _isOn;
};
