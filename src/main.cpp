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
#include <OneWire.h>
#include <DallasTemperature.h>

// WIFI -------------------------------------------
/* Create this file and set those variables in it: */
// const char *ssid = YOUR_SSID;
// const char *password = YOUR_PASSWORD;
// #define SECRET "xxxxx"
#include "ssid_password.h"

// MQTT -------------------------------------------
/* Create this file and set those variables in it: */
// const char *mqttUser = "esp32alarm";
// const char *mqttPassword = "p32/72alarm";
// const char *mqttServer = "192.168.1.99";
// const int mqttPort = 1883;
#include "mqtt.h"

// HC-SR501 PIN ------------------------------------
#define motionSensor 27

// DS18B20 BUS PIN ---------------------------------
#define ONE_WIRE_BUS 33

// BUZZER PIN --------------------------------------
#define buzzer 25

// OBJECT INSTANCES --------------------------------
AsyncWebServer server(80);
HTTPClient httpClient;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttServer, mqttPort, wifiClient); // Arguments are found in "mqtt.h"
// Setup communication for OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature lib
DallasTemperature tempSensor(&oneWire);

// Other values ------------------------------------
bool settingAlarm = false;
bool alarmSet = false;
const int NBR_MEASUREMENTS = 10;
int count = 0;
String HTML_index_file; // HTML & CSS contents which display on web server

int lastValue = 0;
unsigned long prevTime;

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "404! Page not found");
}

void setup()
{
    Serial.begin(115200);
    pinMode(buzzer, OUTPUT);
    pinMode(motionSensor, INPUT);
    tempSensor.begin();

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

    prevTime = millis();

    Serial.println("ESP32 Alarm is ready to use!");
}

void mqttConnect()
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
}

void loop()
{
    const int TEMP_READING_DELAY = 120000; // in ms

    if (millis() - prevTime >= TEMP_READING_DELAY)
    {
        prevTime = millis();
        mqttConnect();
        tempSensor.requestTemperatures(); // Method to get temperatures
        // We use the function ByIndex to get the temperature from the first and only sensor.
        float tempC = tempSensor.getTempCByIndex(0);
        // Check if reading was successful
        bool sent;
        if (tempC != DEVICE_DISCONNECTED_C)
        {
            Serial.printf("Temp: %.3f°C\n", tempC);
            sent = mqttClient.publish("home/esp32/temperature", String(tempC).c_str());
        }
        else
        {
            Serial.println("Couldn't read DS18B20");
            sent = mqttClient.publish("home/esp32/temperature", "Couldn't read DS18B20");
        }
        if (!sent)
        {
            Serial.println("Couldn't send to MQTT!");
        }
    }

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
        /********************** Readings testing with HS-SR501 ***************************/
        int motionDetected = digitalRead(motionSensor);
        if (motionDetected != lastValue)
        {
            mqttConnect();
            bool sent;
            if (motionDetected == HIGH)
            {
                Serial.println("New motion detected!");
                sent = mqttClient.publish("home/esp32/alarm", "New movement");
            }
            else
            {
                Serial.println("Motion stopped");
                sent = mqttClient.publish("home/esp32/alarm", "End of movement");
            }
            lastValue = motionDetected;
            if (!sent)
            {
                Serial.println("Couldn't send to MQTT!");
            }
        }
        /*********************************************************************************/
    }
}