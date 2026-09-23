#pragma once

#include <math.h>

class DynamicAccelerationFilter
{
public:
    explicit DynamicAccelerationFilter(float gravityTrackingAlpha)
        : _alpha(gravityTrackingAlpha), _gravityX(0.0f), _gravityY(0.0f), _gravityZ(0.0f)
    {
    }

    void Seed(float xG, float yG, float zG)
    {
        _gravityX = xG;
        _gravityY = yG;
        _gravityZ = zG;
    }

    float Update(float xG, float yG, float zG)
    {
        float dynamicX = xG - _gravityX;
        float dynamicY = yG - _gravityY;
        float dynamicZ = zG - _gravityZ;
        _gravityX += _alpha * dynamicX;
        _gravityY += _alpha * dynamicY;
        _gravityZ += _alpha * dynamicZ;
        return sqrtf(dynamicX * dynamicX + dynamicY * dynamicY + dynamicZ * dynamicZ);
    }

private:
    float _alpha;
    float _gravityX;
    float _gravityY;
    float _gravityZ;
};
