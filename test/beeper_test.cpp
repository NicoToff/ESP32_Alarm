#include <Arduino.h>

#define echoPin 33
#define trigPin 32
#define buzzer 25
#define NBR_MEASUREMENTS 10

long duration = 0;
int distance = 0;
int count = 0;

void setup()
{
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
    pinMode(buzzer, OUTPUT);
    Serial.begin(9600); // Starts the serial communication
}

int distanceMeasurement(int nbr_measurements)
{
    int duration = 0;
    for (size_t i = 0; i < nbr_measurements; i++)
    {
        // Clears the trigPin
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        // Sets the trigPin on HIGH state for 10 micro seconds
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        // Reads the echoPin, returns the sound wave travel time in microseconds
        duration += pulseIn(echoPin, HIGH);
    }
    duration /= nbr_measurements;

    // Calculating the distance
    return duration * 0.034 / 2;
}

void display(int distance, int count)
{
    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" ; Count: ");
    Serial.println(count);
}

void loop()
{
    distance = distanceMeasurement(NBR_MEASUREMENTS);
    display(distance, count);

    if (distance < 30 && count <= 5)
        count++;
    else if (count > 0)
        count--;

    while (count >= 5)
    {
        Serial.print("BEEEEP ... ");
        digitalWrite(buzzer, HIGH);
        distance = distanceMeasurement(NBR_MEASUREMENTS);
        display(distance, count);
        if (distance > 100)
        {
            count--;
        }
        delay(100);
    }
    digitalWrite(buzzer, LOW);

    delay(100);
}