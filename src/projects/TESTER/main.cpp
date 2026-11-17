#include <Arduino.h>
#include <transmitter/configs/wifi_module.h>

#define PIN_MODEM 3
#define PIN_CNT 8

static const uint8_t MAX_PINS = 32;
static uint8_t activePins[MAX_PINS];
static uint8_t activeCount = 0;

static bool inList(const uint8_t *list, uint8_t count, uint8_t pin)
{
    for (uint8_t i = 0; i < count; i++)
        if (list[i] == pin)
            return true;
    return false;
}

/**
 * Turn on the pins in `pins`, turn off every other pin that was on.
 */
void applyPins(const uint8_t *pins, uint8_t count)
{
    // Turn off previously active pins that are not in the new list
    for (uint8_t i = 0; i < activeCount; i++)
        if (!inList(pins, count, activePins[i]))
            digitalWrite(activePins[i], HIGH);

    // Turn on the requested pins
    for (uint8_t i = 0; i < count; i++)
    {
        pinMode(pins[i], OUTPUT);
        digitalWrite(pins[i], LOW);
    }

    // Remember the new active set
    for (uint8_t i = 0; i < count; i++)
        activePins[i] = pins[i];
    activeCount = count;
}

/**
 * @brief Payload format (one command per line)
 * 1,16,23 means:
 * Turn on GPIO 1, 16, and 23.
 * Turn off other GPIO.
 */
void handleCommand(String line)
{
    line.trim();
    if (line.length() == 0)
        return;

    uint8_t pins[MAX_PINS];
    uint8_t count = 0;
    int start = 0;

    while (start <= (int)line.length() && count < MAX_PINS)
    {
        int comma = line.indexOf(',', start);
        if (comma < 0)
            comma = line.length();

        String token = line.substring(start, comma);
        token.trim();
        if (token.length() > 0)
            pins[count++] = (uint8_t)token.toInt();

        start = comma + 1;
    }

    applyPins(pins, count);
}

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_CNT, OUTPUT);
    pinMode(PIN_MODEM, OUTPUT);
}

void loop()
{
    // if (Serial.available())
    // {
    //     String line = Serial.readStringUntil('\n');
    //     handleCommand(line);
    // }

    static uint32_t prevLog = millis();
    if (millis() - prevLog > 3000)
    {
        prevLog = millis();
        Serial.println("Heartbeat");
    }

    digitalWrite(PIN_CNT, LOW); // Hold controller always ON

    static uint32_t prevModem = millis();
    static uint32_t delayModem = 60000;
    if (millis() - prevModem > delayModem)
    {
        prevModem = millis();
        if (delayModem == 60000)
        {
            digitalWrite(PIN_MODEM, LOW);
            delayModem = 60000 * 5;
        }
        else
        {
            digitalWrite(PIN_MODEM, HIGH);
            delayModem = 60000;
        }
    }
}