/* ****************************
    Created by Nicolas Toffolo
              2022
******************************* */

#include <Arduino.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// Create this file and create those variables in it:
// const char *ssid = YOUR_SSID;
// const char *password = YOUR_PASSWORD;
// #define SECRET "xxxxx"

#include "ssid_password.h"
#include "mqtt.h"

#define echoPin 33
#define trigPin 32
#define NBR_MEASUREMENTS 10
int count = 0;

#define buzzer 25

AsyncWebServer server(80);
HTTPClient httpClient;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttServer, mqttPort, wifiClient); // Arguments found in "mqtt.h"

const char *PARAM_MESSAGE = "message";

// HTML & CSS contents which display on web server
String HTML_index_file;

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "404! Pag not found");
}

void setup()
{
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
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
        Serial.println(HTML_index_file);
    } while (!HTTP_CODE_OK);
    Serial.println(" = OK");
    httpClient.end(); // Frees the resources

    // Server routes on ESP32 --------------------------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", HTML_index_file); });

    // Examples of parametized GET and POST requests
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

    server.on("/alarm", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                Serial.println("BEEEP!");
                digitalWrite(buzzer, HIGH);
                request->send(200); });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request)
              { digitalWrite(buzzer, LOW);
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

    // MQTT ---------------------------------------------------------------------------
    Serial.print("Connecting to MQTT");
    while (!mqttClient.connect("esp32", mqttUser, mqttPassword))
    {
        Serial.print(".");
        delay(250);
    }
    Serial.println("\nConnected!");
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

void loop()
{
    int reading = ultrasonicReading(NBR_MEASUREMENTS);
    Serial.printf("Sonic reading: %d\n", reading);
    bool sent = mqttClient.publish("home/alarm/reading", String(reading).c_str());
    Serial.printf("Sent to MQTT = %d\n", sent);
}