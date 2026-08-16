#if !defined(ADS1115_MODULE_H)
#define ADS1115_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include "ADS1115.h"

class ADS1115Module
{
private:
    TwoWire *_wire;
    ADS1115 _ads;
    uint8_t _currentChannel = 255; // Track current channel to avoid redundant switches

public:
    ADS1115Module(const uint8_t address, TwoWire *wire)
        : _ads(ADS1115(address)), _wire(wire) {}
    ~ADS1115Module() {}

    bool begin(const uint8_t gain = ADS1115_PGA_4P096)
    {
        if (!_ads.testConnection())
            return false;

        _ads.initialize();
        _ads.setMode(ADS1115_MODE_CONTINUOUS); // Always converting
        _ads.setRate(ADS1115_RATE_128);
        _ads.setGain(gain);

        return true;
    }

    float read(uint8_t channel)
    {
        // Only set multiplexer if channel changed
        if (_currentChannel != channel)
        {
            switch (channel)
            {
            case 0:
                _ads.setMultiplexer(ADS1115_MUX_P0_NG);
                break;
            case 1:
                _ads.setMultiplexer(ADS1115_MUX_P1_NG);
                break;
            case 2:
                _ads.setMultiplexer(ADS1115_MUX_P2_NG);
                break;
            case 3:
                _ads.setMultiplexer(ADS1115_MUX_P3_NG);
                break;
            default:
                return 0;
            }

            _currentChannel = channel;

            // In continuous mode, MUX switching takes longer to settle
            // because the ADC is actively converting. Wait for new channel's
            // first conversion to complete (~7.8ms at RATE_128) + MUX settle
            delayMicroseconds(500); // Initial MUX settle
            // Let one full conversion cycle complete on new channel
            // At RATE_128: 7.8ms per sample
            delay(10); // Safe to ensure first result on new channel is ready
        }

        // In continuous mode, just read—conversion is always running
        return _ads.getMilliVolts(false); // false = don't wait, result ready
    }
};

#endif // ADS1115_MODULE_H