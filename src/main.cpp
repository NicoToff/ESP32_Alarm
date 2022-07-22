/* ****************************
    Created by Nicolas Toffolo
              2022
******************************* */

// LIB IMPORTS ------------------------------------
#include <Arduino.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// WIFI -------------------------------------------
// Create this file and set those variables in it:
// const char *ssid = YOUR_SSID;
// const char *password = YOUR_PASSWORD;
// #define SECRET "xxxxx"
#include "ssid_password.h"

// MQTT -------------------------------------------
// Create this file and set those variables in it:
// const char *mqttUser = "esp32alarm";
// const char *mqttPassword = "p32/72alarm";
// const char *mqttServer = "192.168.1.99";
// const int mqttPort = 1883;
#include "mqtt.h"

// HC-SR04 PINS ------------------------------------
#define echoPin 33
#define trigPin 32

// HC-SR501 PIN ------------------------------------
#define motionSensor 27

// BUZZER PIN --------------------------------------
#define buzzer 25

// OBJECT INSTANCES --------------------------------
AsyncWebServer server(80);
HTTPClient httpClient;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttServer, mqttPort, wifiClient); // Arguments are found in "mqtt.h"

// Other values ------------------------------------
bool settingAlarm = false;
bool alarmSet = false;
const int NBR_MEASUREMENTS = 10;
int count = 0;
String HTML_index_file;                // HTML & CSS contents which display on web server
const char *PARAM_MESSAGE = "message"; // used for tests

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "404! Page not found");
}

void setup()
{
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(buzzer, OUTPUT);

    // Setting Static IP
    IPAddress local_IP(192, 168, 1, 191);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);
    IPAddress secondaryDNS(8, 8, 4, 4);

    // Configures static IP address
    while (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
        Serial.println("Static IP Failed to configure! Trying again...");
    }

    // Connecting to WiFi
    // and looping in case it doesn't work
    Serial.println("Connecting to WiFi network...");
    uint8_t wifiResult;
    do
    {
        WiFi.begin(ssid, password);
        wifiResult = WiFi.waitForConnectResult();
        if (wifiResult != WL_CONNECTED)
        {
            Serial.println("WiFi failed! Trying again...");
        }
    } while (wifiResult != WL_CONNECTED);

    Serial.print("WiFi connected\nIP Address: ");
    Serial.println(WiFi.localIP());

    // UI ----------------------------------------------------------------------------------
    // Fetching HTML index data stored on GitHub instead of a clunky String inside this file
    int httpReturnCode = 0;
    do
    {
        httpClient.begin("https://raw.githubusercontent.com/NicoToff/ESP32_Alarm/main/src/index.html");
        httpReturnCode = httpClient.GET();
        Serial.print("HTTP Code: ");
        Serial.print(httpReturnCode);
        HTML_index_file = httpClient.getString();
        // Serial.println(HTML_index_file);
    } while (!HTTP_CODE_OK);
    Serial.println(" = OK");
    httpClient.end(); // Frees the resources once it's done

    // Server routes on ESP32 --------------------------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", HTML_index_file); });

    /*************** Examples of parametized GET and POST requests *************************/
    // // Send a GET request to <IP>/get?message=<message>
    // server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {
    //               String message;
    //               if (request->hasParam(PARAM_MESSAGE)) {
    //                   message = request->getParam(PARAM_MESSAGE)->value();
    //               } else {
    //                   message = "No message sent";
    //               }
    //               request->send(200, "text/plain", "Hello, GET: " + message); });

    // // Send a POST request to <IP>/post with a form field message set to <message>
    // server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request)
    //           {
    //               String message;
    //               if (request->hasParam(PARAM_MESSAGE, true)) {
    //                   message = request->getParam(PARAM_MESSAGE, true)->value();
    //               } else {
    //                   message = "No message sent";
    //               }
    //               request->send(200, "text/plain", "Hello, POST: " + message); });
    /* *********************************************************************************** */

    server.on("/alarm", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                if(!settingAlarm) {
                    Serial.println("Alarm setting...");
                    settingAlarm = true;
                } else {
                    Serial.println("Alarm is setting. Press STOP to stop.");
                }
                request->send(200); });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request)
              { if(alarmSet) {
                    Serial.println("Alarm stopped!");
                    alarmSet = false;
                }
                if(settingAlarm) {
                    Serial.println("Alarm setting interrupted!");
                    settingAlarm = false;
                }

                request->send(200); });

    server.on("/pw", HTTP_POST, [](AsyncWebServerRequest *request)
              {   Serial.println("Receiving PW ...");
                  String password = request->getParam(0)->value();
                  Serial.print("Received: ");
                  Serial.println(password);
                  if (password.equals(SECRET)) {
                      Serial.println("Password is valid");
                      request->send(200, "application/json", "OK");
                  } else {
                      Serial.println("Password is invalid");
                      request->send(200, "application/json", "WRONG");
                  } });

    server.onNotFound(notFound);

    server.begin();

    Serial.println("ESP32 Alarm is ready to use!");
}

int ultrasonicReading(int nbr_measurements)
{
    int totalMeasurements = 0;
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
        totalMeasurements += pulseIn(echoPin, HIGH);
    }
    int averageMeasurement = totalMeasurements /= nbr_measurements;

    // Returning an average of "nbr_measurements" readings
    return averageMeasurement;
}

int lastValue = 0;

void loop()
{
    if (settingAlarm && !alarmSet)
    {
        const int TIME_DELAY = 10;
        for (size_t i = 0; i < TIME_DELAY; i++)
        {
            delay(1000);
            if (!settingAlarm)
                break;
            Serial.printf("Alarm on in %d...\n", TIME_DELAY - i);
        }
        if (settingAlarm)
        {
            Serial.println("Alarm set!");
            settingAlarm = false;
            alarmSet = true;
        }
    }

    if (alarmSet)
    {
        // MQTT Connection -----------------------------------------------------------------
        while (!mqttClient.connected())
        {
            Serial.print("Connecting to MQTT...");
            while (!mqttClient.connect("esp32", mqttUser, mqttPassword))
            {
                Serial.print(".");
                delay(1);
            }
            Serial.println("\nConnected!");
        }

        /********************** Readings testing with HS-SR501 ***************************/
        int motionDetected = digitalRead(motionSensor);
        if (motionDetected != lastValue)
        {
            bool sent;
            if (motionDetected == HIGH)
            {
                Serial.println("New motion detected!");
                sent = mqttClient.publish("home/alarm/reading", "New movement");
            }
            else
            {
                Serial.println("Motion stopped");
                sent = mqttClient.publish("home/alarm/reading", "End of movement");
            }
            lastValue = motionDetected;
            if (!sent)
            {
                Serial.println("Couldn't send to MQTT!");
            }
        }
        /*********************************************************************************/

        /********************** Readings testing with HS-SR04 ****************************/
        // int reading = ultrasonicReading(NBR_MEASUREMENTS);
        // Serial.printf("Sonic reading: %d\n", reading);
        // bool sent = mqttClient.publish("home/alarm/reading", String(reading).c_str());
        // if (!sent)
        // {
        //     Serial.println("Couldn't send to MQTT!");
        // }
        /*********************************************************************************/
    }
}