#pragma once

#include <Arduino.h>
#include <Wire.h>

class Mpu6050
{
public:
    explicit Mpu6050(TwoWire& wire, uint8_t address = DEFAULT_ADDRESS)
        : _wire(wire), _address(address)
    {
    }

    void SetAddress(uint8_t address)
    {
        _address = address;
    }

    bool Begin()
    {
        if (!WriteRegister(REGISTER_POWER_MANAGEMENT_1, DEVICE_RESET))
        {
            return false;
        }
        delay(100);
        if (!WriteRegister(REGISTER_POWER_MANAGEMENT_1, CLOCK_SOURCE_X_GYROSCOPE_PLL))
        {
            return false;
        }
        delay(10);
        return WriteRegister(REGISTER_SAMPLE_RATE_DIVIDER, SAMPLE_RATE_1KHZ)
            && WriteRegister(REGISTER_CONFIGURATION, LOW_PASS_FILTER_184HZ)
            && WriteRegister(REGISTER_ACCELEROMETER_CONFIGURATION, ACCELEROMETER_RANGE_8G);
    }

    uint8_t ReadWhoAmI()
    {
        uint8_t value = 0;
        ReadRegisters(REGISTER_WHO_AM_I, &value, 1);
        return value;
    }

    bool ReadAcceleration(float& xG, float& yG, float& zG)
    {
        uint8_t buffer[6];
        if (!ReadRegisters(REGISTER_ACCELEROMETER_X_HIGH, buffer, sizeof(buffer)))
        {
            return false;
        }
        xG = ToInt16(buffer[0], buffer[1]) / COUNTS_PER_G;
        yG = ToInt16(buffer[2], buffer[3]) / COUNTS_PER_G;
        zG = ToInt16(buffer[4], buffer[5]) / COUNTS_PER_G;
        return true;
    }

private:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x68;
    static constexpr uint8_t REGISTER_SAMPLE_RATE_DIVIDER = 0x19;
    static constexpr uint8_t REGISTER_CONFIGURATION = 0x1A;
    static constexpr uint8_t REGISTER_ACCELEROMETER_CONFIGURATION = 0x1C;
    static constexpr uint8_t REGISTER_ACCELEROMETER_X_HIGH = 0x3B;
    static constexpr uint8_t REGISTER_POWER_MANAGEMENT_1 = 0x6B;
    static constexpr uint8_t REGISTER_WHO_AM_I = 0x75;
    static constexpr uint8_t DEVICE_RESET = 0x80;
    static constexpr uint8_t CLOCK_SOURCE_X_GYROSCOPE_PLL = 0x01;
    static constexpr uint8_t SAMPLE_RATE_1KHZ = 0x00;
    static constexpr uint8_t LOW_PASS_FILTER_184HZ = 0x01;
    static constexpr uint8_t ACCELEROMETER_RANGE_8G = 0x10;
    static constexpr float COUNTS_PER_G = 4096.0f;

    static int16_t ToInt16(uint8_t high, uint8_t low)
    {
        return (int16_t)(((uint16_t)high << 8) | low);
    }

    bool WriteRegister(uint8_t registerAddress, uint8_t value)
    {
        _wire.beginTransmission(_address);
        _wire.write(registerAddress);
        _wire.write(value);
        return _wire.endTransmission() == 0;
    }

    bool ReadRegisters(uint8_t registerAddress, uint8_t* buffer, uint8_t length)
    {
        _wire.beginTransmission(_address);
        _wire.write(registerAddress);
        if (_wire.endTransmission(false) != 0)
        {
            return false;
        }
        if (_wire.requestFrom((int)_address, (int)length) != length)
        {
            return false;
        }
        for (uint8_t index = 0; index < length; index++)
        {
            buffer[index] = _wire.read();
        }
        return true;
    }

    TwoWire& _wire;
    uint8_t _address;
};
