#pragma once

#include <stdint.h>

#include "TapEvent.h"

class TapDetector
{
public:
    TapDetector(float thresholdG,
                uint8_t tapsToActivate,
                uint32_t debounceMilliseconds,
                uint32_t minimumGapMilliseconds,
                uint32_t maximumGapMilliseconds,
                uint32_t lockoutMilliseconds)
        : _thresholdG(thresholdG),
          _tapsToActivate(tapsToActivate),
          _debounceMilliseconds(debounceMilliseconds),
          _minimumGapMilliseconds(minimumGapMilliseconds),
          _maximumGapMilliseconds(maximumGapMilliseconds),
          _lockoutMilliseconds(lockoutMilliseconds),
          _armed(false),
          _inTapWindow(false),
          _hasPreviousTap(false),
          _tapCount(0),
          _tapStartMilliseconds(0),
          _previousTapMilliseconds(0),
          _lockoutUntilMilliseconds(0),
          _lastPeakG(0.0f),
          _lastGapMilliseconds(0)
    {
    }

    TapEvent Update(uint32_t nowMilliseconds, float dynamicG)
    {
        if (_inTapWindow)
        {
            if (dynamicG > _lastPeakG)
            {
                _lastPeakG = dynamicG;
            }
            if (nowMilliseconds - _tapStartMilliseconds < _debounceMilliseconds)
            {
                return TapEvent::None;
            }
            _inTapWindow = false;
            return FinalizeTap(nowMilliseconds);
        }

        if (dynamicG < _thresholdG * REARM_RATIO)
        {
            _armed = true;
        }

        if ((int32_t)(nowMilliseconds - _lockoutUntilMilliseconds) < 0)
        {
            return TapEvent::None;
        }

        if (_armed && dynamicG > _thresholdG)
        {
            _armed = false;
            _inTapWindow = true;
            _tapStartMilliseconds = nowMilliseconds;
            _lastPeakG = dynamicG;
        }
        return TapEvent::None;
    }

    float LastPeakG() const
    {
        return _lastPeakG;
    }

    uint32_t LastGapMilliseconds() const
    {
        return _lastGapMilliseconds;
    }

    uint8_t TapCount() const
    {
        return _tapCount;
    }

    uint8_t TapsToActivate() const
    {
        return _tapsToActivate;
    }

private:
    static constexpr float REARM_RATIO = 0.5f;

    TapEvent FinalizeTap(uint32_t nowMilliseconds)
    {
        _lastGapMilliseconds = _hasPreviousTap ? _tapStartMilliseconds - _previousTapMilliseconds : 0;
        bool continuesSequence = _hasPreviousTap
            && _lastGapMilliseconds >= _minimumGapMilliseconds
            && _lastGapMilliseconds <= _maximumGapMilliseconds;
        _tapCount = continuesSequence ? _tapCount + 1 : 1;
        _hasPreviousTap = true;
        _previousTapMilliseconds = _tapStartMilliseconds;
        if (_tapCount >= _tapsToActivate)
        {
            _hasPreviousTap = false;
            _lockoutUntilMilliseconds = nowMilliseconds + _lockoutMilliseconds;
            return TapEvent::Activate;
        }
        return TapEvent::Tap;
    }

    float _thresholdG;
    uint8_t _tapsToActivate;
    uint32_t _debounceMilliseconds;
    uint32_t _minimumGapMilliseconds;
    uint32_t _maximumGapMilliseconds;
    uint32_t _lockoutMilliseconds;
    bool _armed;
    bool _inTapWindow;
    bool _hasPreviousTap;
    uint8_t _tapCount;
    uint32_t _tapStartMilliseconds;
    uint32_t _previousTapMilliseconds;
    uint32_t _lockoutUntilMilliseconds;
    float _lastPeakG;
    uint32_t _lastGapMilliseconds;
};
