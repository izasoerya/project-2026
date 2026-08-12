#ifndef HX711_READING_H
#define HX711_READING_H

#include <Arduino.h>
#include "HX711.h"

int8_t weightBegin(HX711 &sc, uint8_t dataPin, uint8_t clockPin)
{
    sc.begin(dataPin, clockPin);
    sc.set_scale(0);
    sc.set_offset(0);
    // if (dataPin == 4)
    // {
    //     sc.set_offset(478858.00);
    // }
    // else if (dataPin == 5)
    // {
    //     sc.set_offset(-233375.00);
    // }
    // sc.tare();
    // Serial.print("Data: ");
    // Serial.print(dataPin);
    // Serial.print(", get tare: ");
    // Serial.println(sc.get_tare());

    // TODO: DUNNO HOW TO DETECT IF SENSOR FAILING
    return 1;
}

float weightReading(HX711 &scale)
{
    return scale.read_average(15);
}

bool calibrate(HX711 &scale)
{
    Serial.println("\n\nCALIBRATION\n===========");
    Serial.println("remove all weight from the loadcell");
    //  flush Serial input
    while (Serial.available())
        Serial.read();

    Serial.println("and press enter\n");
    while (Serial.available() == 0)
        ;

    Serial.println("Determine zero weight offset");
    //  average 20 measurements.
    scale.tare(20);
    int32_t offset = scale.get_offset();

    Serial.print("OFFSET: ");
    Serial.println(offset);
    Serial.println();

    Serial.println("place a weight on the loadcell");
    //  flush Serial input
    while (Serial.available())
        Serial.read();

    Serial.println("enter the weight in (whole) grams and press enter");
    uint32_t weight = 0;
    while (Serial.peek() != '\n')
    {
        if (Serial.available())
        {
            char ch = Serial.read();
            if (isdigit(ch))
            {
                weight *= 10;
                weight = weight + (ch - '0');
            }
        }
    }
    Serial.print("WEIGHT: ");
    Serial.println(weight);
    scale.calibrate_scale(weight, 20);

    Serial.print("SCALE:  ");
    Serial.println(scale.get_scale(), 6);

    Serial.print("\nuse scale.set_offset(");
    Serial.print(offset);
    Serial.print("); and scale.set_scale(");
    Serial.print(scale.get_scale(), 6);
    Serial.print(");\n");
    Serial.println("in the setup of your project");

    Serial.println("\n\n");

    return 1;
}

#endif