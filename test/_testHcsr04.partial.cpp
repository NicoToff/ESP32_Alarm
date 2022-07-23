// HC-SR04 PINS ------------------------------------
#define echoPin 33
#define trigPin 32

// int ultrasonicReading(int nbr_measurements)
// {
//     int totalMeasurements = 0;
//     for (size_t i = 0; i < nbr_measurements; i++)
//     {
//         // Clears the trigPin
//         digitalWrite(trigPin, LOW);
//         delayMicroseconds(2);
//         // Sets the trigPin on HIGH state for 10 micro seconds
//         digitalWrite(trigPin, HIGH);
//         delayMicroseconds(10);
//         digitalWrite(trigPin, LOW);
//         // Reads the echoPin, returns the sound wave travel time in microseconds
//         totalMeasurements += pulseIn(echoPin, HIGH);
//     }
//     int averageMeasurement = totalMeasurements /= nbr_measurements;

//     // Returning an average of "nbr_measurements" readings
//     return averageMeasurement;
// }

// void setup()
// {
//     pinMode(trigPin, OUTPUT);
//     pinMode(echoPin, INPUT);
// }
/********************** Readings testing with HS-SR04 ****************************/
// int reading = ultrasonicReading(NBR_MEASUREMENTS);
// Serial.printf("Sonic reading: %d\n", reading);
// bool sent = mqttClient.publish("home/alarm/reading", String(reading).c_str());
// if (!sent)
// {
//     Serial.println("Couldn't send to MQTT!");
// }
/*********************************************************************************/