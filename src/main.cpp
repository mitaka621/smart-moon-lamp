#include <Arduino.h>
#include <Wire.h>

#include "DynamicAccelerationFilter.h"
#include "Mpu6050.h"
#include "Relay.h"
#include "RelayStateStorage.h"
#include "TapDetector.h"
#include "TapEvent.h"

constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint32_t I2C_FREQUENCY_HZ = 400000;
constexpr uint8_t MPU6050_ADDRESSES[] = {0x68, 0x69};

constexpr uint8_t RELAY_PIN = 4;
constexpr bool RELAY_ACTIVE_LOW = true;
constexpr bool RELAY_DEFAULT_IS_ON = false;

constexpr uint32_t SAMPLE_INTERVAL_MICROSECONDS = 2000;
constexpr float GRAVITY_TRACKING_ALPHA = 0.002f;

constexpr float TAP_THRESHOLD_G = 0.5f;
constexpr uint8_t TAPS_TO_ACTIVATE = 3;
constexpr uint32_t TAP_DEBOUNCE_MILLISECONDS = 80;
constexpr uint32_t TAP_MINIMUM_GAP_MILLISECONDS = 120;
constexpr uint32_t TAP_MAXIMUM_GAP_MILLISECONDS = 500;
constexpr uint32_t ACTIVATE_LOCKOUT_MILLISECONDS = 800;

constexpr bool DEBUG = true;
constexpr uint16_t LOG_EVERY_N_SAMPLES = 1;
constexpr float LOG_ONLY_WHEN_DYNAMIC_ABOVE_G = 0.0f;

Mpu6050 mpu6050(Wire);
Relay relay(RELAY_PIN, RELAY_ACTIVE_LOW);
RelayStateStorage relayStateStorage;
DynamicAccelerationFilter dynamicAccelerationFilter(GRAVITY_TRACKING_ALPHA);
TapDetector tapDetector(TAP_THRESHOLD_G,
                        TAPS_TO_ACTIVATE,
                        TAP_DEBOUNCE_MILLISECONDS,
                        TAP_MINIMUM_GAP_MILLISECONDS,
                        TAP_MAXIMUM_GAP_MILLISECONDS,
                        ACTIVATE_LOCKOUT_MILLISECONDS);

uint32_t nextSampleMicroseconds = 0;
uint32_t sampleCounter = 0;

void RestoreRelayState()
{
    relayStateStorage.Begin();
    relay.Begin(relayStateStorage.Load(RELAY_DEFAULT_IS_ON));
}

void BeginSerial()
{
    if (!DEBUG)
    {
        return;
    }
    Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);
#endif
}

void ScanI2cBus()
{
    if (!DEBUG)
    {
        return;
    }
    Serial.print("I2C devices found:");
    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.printf(" 0x%02X", address);
        }
    }
    Serial.println();
}

bool TryBeginSensor()
{
    for (uint8_t address : MPU6050_ADDRESSES)
    {
        mpu6050.SetAddress(address);
        if (mpu6050.Begin())
        {
            if (DEBUG)
            {
                Serial.printf("MPU6050 ready at 0x%02X, WHO_AM_I=0x%02X\n", address, mpu6050.ReadWhoAmI());
            }
            return true;
        }
    }
    return false;
}

void BeginSensor()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQUENCY_HZ);
    while (!TryBeginSensor())
    {
        if (DEBUG)
        {
            Serial.println("MPU6050 not responding at 0x68 or 0x69");
        }
        ScanI2cBus();
        delay(1000);
    }

    float xG;
    float yG;
    float zG;
    while (!mpu6050.ReadAcceleration(xG, yG, zG))
    {
        delay(10);
    }
    dynamicAccelerationFilter.Seed(xG, yG, zG);
}

void HandleTapEvent(TapEvent tapEvent)
{
    if (tapEvent == TapEvent::None)
    {
        return;
    }
    if (tapEvent == TapEvent::Activate)
    {
        relay.Toggle();
        relayStateStorage.Save(relay.IsOn());
    }
    if (!DEBUG)
    {
        return;
    }
    Serial.printf("[TAP %u/%u] peak=%.2fg gap=%ums",
                  (unsigned)tapDetector.TapCount(),
                  (unsigned)tapDetector.TapsToActivate(),
                  tapDetector.LastPeakG(),
                  (unsigned)tapDetector.LastGapMilliseconds());
    if (tapEvent == TapEvent::Activate)
    {
        Serial.printf(" [ACTIVATE] relay %s, state saved", relay.IsOn() ? "ON" : "OFF");
    }
    Serial.println();
}

void LogSample(float xG, float yG, float zG, float dynamicG)
{
    if (!DEBUG)
    {
        return;
    }
    sampleCounter++;
    if (sampleCounter % LOG_EVERY_N_SAMPLES != 0)
    {
        return;
    }
    if (dynamicG < LOG_ONLY_WHEN_DYNAMIC_ABOVE_G)
    {
        return;
    }
    Serial.printf("x:%.3f y:%.3f z:%.3f dynamic:%.3f threshold:%.2f relay:%d\n",
                  xG, yG, zG, dynamicG, TAP_THRESHOLD_G, relay.IsOn() ? 1 : 0);
}

void setup()
{
    RestoreRelayState();
    BeginSerial();
    if (DEBUG)
    {
        Serial.printf("boot, relay restored from NVS: %s\n", relay.IsOn() ? "ON" : "OFF");
    }
    BeginSensor();
    if (DEBUG)
    {
        Serial.printf("threshold=%.2fg taps=%u debounce=%ums gap=%u-%ums lockout=%ums\n",
                      TAP_THRESHOLD_G,
                      (unsigned)TAPS_TO_ACTIVATE,
                      (unsigned)TAP_DEBOUNCE_MILLISECONDS,
                      (unsigned)TAP_MINIMUM_GAP_MILLISECONDS,
                      (unsigned)TAP_MAXIMUM_GAP_MILLISECONDS,
                      (unsigned)ACTIVATE_LOCKOUT_MILLISECONDS);
    }
    nextSampleMicroseconds = micros();
}

void loop()
{
    if ((int32_t)(micros() - nextSampleMicroseconds) < 0)
    {
        return;
    }
    nextSampleMicroseconds += SAMPLE_INTERVAL_MICROSECONDS;

    float xG;
    float yG;
    float zG;
    if (!mpu6050.ReadAcceleration(xG, yG, zG))
    {
        if (DEBUG)
        {
            Serial.println("MPU6050 read failed");
        }
        delay(500);
        nextSampleMicroseconds = micros();
        return;
    }

    float dynamicG = dynamicAccelerationFilter.Update(xG, yG, zG);
    HandleTapEvent(tapDetector.Update(millis(), dynamicG));
    LogSample(xG, yG, zG, dynamicG);
}
