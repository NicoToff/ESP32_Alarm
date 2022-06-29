#include <Arduino.h>
#include <SoftwareSerial.h>

#define rx 16
#define tx 17

// Create software serial object to communicate with SIM800L
SoftwareSerial sim800(tx, rx); // SIM800L Tx & Rx is connected to ESP32 #17 & #16

void updateSerial()
{
    delay(500);
    while (Serial.available())
    {
        sim800.write(Serial.read()); // Forward what Serial received to Software Serial Port
    }
    while (sim800.available())
    {
        Serial.write(sim800.read()); // Forward what Software Serial received to Serial Port
    }
}

void setup()
{
    // Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
    Serial.begin(9600);

    // Begin serial communication with ESP32 and SIM800L
    sim800.begin(9600);

    Serial.println("Initializing...");
    delay(1000);

    sim800.println("AT"); // Once the handshake test is successful, it will back to OK
    updateSerial();
    sim800.println("AT+CSQ"); // Signal quality test, value range is 0-31 , 31 is the best
    updateSerial();
    sim800.println("AT+CCID"); // Read SIM information to confirm whether the SIM is plugged
    updateSerial();
    sim800.println("AT+CREG?"); // Check whether it has registered in the network
    updateSerial();
}

void loop()
{
    updateSerial();
}