#if !defined(DEBUG_TOOLS_H)
#define DEBUG_TOOLS_H

#include <Arduino.h>
#include <Wire.h>
#include <WebSerial.h>
#include <sensor/configs/modbus_sensor.h>

class DebugTools
{
    static void i2cScanner(TwoWire &wire, WebSerialClass &w)
    {
        byte error, address;
        uint8_t nDevices;

        nDevices = 0;
        for (address = 1; address < 127; address++)
        {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            if (error == 0)
            {
                w.print("I2C device found at address 0x");
                if (address < 16)
                    w.print("0");
                w.println(address, HEX);
                nDevices++;
            }
            else if (error == 4)
            {
                w.print("Unknow error at address 0x");
                if (address < 16)
                {
                    w.print("0");
                }
                w.println(address, HEX);
            }
        }
        if (nDevices == 0)
            w.println("No I2C devices found\n");
    }

    static void i2cScanner(TwoWire &wire, HardwareSerial &s)
    {
        byte error, address;
        uint8_t nDevices;

        nDevices = 0;
        for (address = 1; address < 127; address++)
        {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            if (error == 0)
            {
                s.print("I2C device found at address 0x");
                if (address < 16)
                    s.print("0");
                s.println(address, HEX);
                nDevices++;
            }
            else if (error == 4)
            {
                s.print("Unknow error at address 0x");
                if (address < 16)
                {
                    s.print("0");
                }
                s.println(address, HEX);
            }
        }
        if (nDevices == 0)
            s.println("No I2C devices found\n");
    }

    static void modbusScanner(Stream &src, uint8_t id, uint8_t fc, uint8_t sa, uint8_t la)
    {
        Modbustatics debugSensor(1, "debug sensor", src, sa);

        debugSensor.begin();
        ReadResult x = debugSensor.rawRead();

        if (x.isOk())
            Serial.printf("Success Read: %d", x.value);
        else
            Serial.println(x.errorMessage());
    }
};

#endif // DEBUG_TOOLS_H
