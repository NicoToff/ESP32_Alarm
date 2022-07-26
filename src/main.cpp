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
// Usable object to get temperatures
DallasTemperature tempSensor(&oneWire);

/* Helper constants -------------------------------- */
const bool SEND_TO_MQTT = true;
const bool LOG_TO_CONSOLE = true;

/* Other values ------------------------------------ */
bool settingAlarm = false;
bool alarmSet = false;
const int NBR_MEASUREMENTS = 10;
int count = 0;
// HTML & CSS contents which to on web server
String HTML_index_file;
unsigned long prevTime;

// MQTT Connection -----------------------------------------------------------------
// Tries to connect for 5 seconds, and then stops attempting if it keeps failing.
// Does nothing is already connected to MQTT.
// Returns true if connected to MQTT
bool mqttConnect()
{
    bool connected = mqttClient.connected();
    if (!connected)
    {
        unsigned long startTime = millis();
        Serial.print("Connecting to MQTT...");
        while (!mqttClient.connect("esp32", mqttUser, mqttPassword) && millis() - startTime < 5000)
        {
            Serial.print(".");
            delay(100);
        }
        connected = mqttClient.connected();
        connected ? Serial.println("\nConnected!") : Serial.println("\n FAILED!!!");
    }
    return connected;
}

bool sendToMqtt(String topic, String message)
{
    bool sent = false;
    if (mqttConnect())
    {
        sent = mqttClient.publish(topic.c_str(), message.c_str());
    }
    if (!sent)
    {
        Serial.println("Couldn't send to MQTT!");
    }
    return sent;
}
// A class to deal easily with binary motion sensors, like the HC-SR501.
class MotionSensor
{
private:
    bool movementDetected = false;
    bool lastValue = false;
    int pin = -1;
    String name;

public:
    MotionSensor(int pin, String name = "motionSensor")
    {
        this->pin = pin;
        this->name = name;
    }
    /**
     * @brief Sets the sensor's pin as INPUT
     * @return int The sensor's pin
     */
    int setup()
    {
        pinMode(this->pin, INPUT);
        return this->pin;
    }
    /**
     * @brief Reads the state of the sensor's pin. If it's HIGH, the sensor detected movement.
     *
     * @param logToConsole Set to true if you want a message to be logged with the Serial object (default is false)
     * @param forwardToMqtt Set to true if you want a message to be sent to an MQTT broker (default is false)
     * @param topic The topic for MQTT; it's appended with the name of the sensor (default is "/motionsensor/")
     * @param message The message to be sent to MQTT (default is "Movement")
     * @return true if movement was detected
     */
    bool checkForMovement(bool logToConsole = false, bool forwardToMqtt = false, String topic = "/motionsensor/", String message = "Movement")
    {
        this->movementDetected = digitalRead(this->pin);
        if (this->movementDetected != this->lastValue)
        {
            this->lastValue = this->movementDetected;
            if (this->movementDetected == true)
            {
                if (forwardToMqtt)
                {
                    sendToMqtt(topic + this->name, message);
                }
                if (logToConsole)
                {
                    Serial.println(this->name + " detected movement");
                }
            }
        }
        return this->movementDetected;
    }
    void setName(String name)
    {
        this->name = name;
    }
    String getName()
    {
        return this->name;
    }
};

// HC-SR501 ----------------------------------------------------
MotionSensor motionSensors[] = {
    MotionSensor(27, "HC-SR501_sensor1"),
    MotionSensor(26, "HC-SR501_sensor2"),
    MotionSensor(14, "HC-SR501_sensor3")};

/**
 * @brief Gets temperature given by the sole OneWire temp sensor available.
 *
 * @param logToConsole Set to true if you want a message to be logged with the Serial object (default is false)
 * @param forwardToMqtt Set to true if you want a message to be sent to an MQTT broker (default is false)
 * @param tempCOffset If sensor is inaccurate, you can give an offset value here
 * @return The temperature from the sensor, as a float. Will return (-127 + tempCOffset) if sensor can't be reached
 */
float checkTempC(bool logToConsole = false, bool forwardToMqtt = false, int tempCOffset = 0)
{
    tempSensor.requestTemperatures(); // Method to get temperatures.
    // Function xxxByIndex() to get the temperature from the first (and sole) sensor.
    float tempC = tempSensor.getTempCByIndex(0);
    tempC += tempCOffset;
    if (logToConsole)
    {
        tempC == DEVICE_DISCONNECTED_C + tempCOffset ? Serial.printf("Couldn't reach sensor\n")
                                                     : Serial.printf("Temp: %.3f°C\n", tempC);
    }
    if (forwardToMqtt)
    {
        sendToMqtt("home/esp32/temperature", String(tempC));
    }
    return tempC; // Will return -127 if DEVICE_DISCONNECTED_C
}

/* Buzzer ----------------------------------------------------- */
// Defines the length of beeps
typedef enum
{
    SHORT_BEEP,
    LONG_BEEP
} beep_length_t;

/**
 * @brief Makes the piezo buzzer beep a given number of times.
 * Beeps can be SHORT_BEEP (35 ms) or LONG_BEEP (150 ms) and can go from 1 to 10.
 *
 * @param amount number of beeps (default = 1)
 * @param LENGTH length of the beep(s) (default = SHORT_BEEP)
 */
void beep(int amount = 1, beep_length_t LENGTH = SHORT_BEEP)
{
    if (amount < 1)
        amount = 1;
    else if (amount > 10)
        amount = 10;

    for (int i = 0; i < amount; i++)
    {
        delay(25);
        digitalWrite(buzzer, HIGH);
        delay(LENGTH == LONG_BEEP ? 150 : 35);
        digitalWrite(buzzer, LOW);
        delay(25);
    }
}
// -------------------------------------------------------------

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "404! Page not found");
}

void setup()
{
    Serial.begin(115200);
    pinMode(buzzer, OUTPUT);
    for (int i = 0; i < sizeof(motionSensors) / sizeof(motionSensors[0]); i++)
    {
        motionSensors[i].setup();
    }
    tempSensor.begin();

    // WiFi --------------------------------------------------------------------------------
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
    } while (httpReturnCode != HTTP_CODE_OK);
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
                    Serial.println("Alarm is set or setting. Press STOP to stop.");
                }
                request->send(200); });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request)
              { if(alarmSet) {
                    Serial.println("Alarm stopped!");
                    alarmSet = false;
                    mqttConnect();
                    mqttClient.publish("home/esp32/alarm/status", "ALARM OFF");
                    request->send(200);
                }
                else if(settingAlarm) {
                    Serial.println("Alarm setting interrupted!");
                    settingAlarm = false;
                    mqttConnect();
                    mqttClient.publish("home/esp32/alarm/status", "ALARM OFF");
                    request->send(200);
                }                
                request->send(204); });

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

    // Gets temperature a first time, then every so often
    checkTempC(LOG_TO_CONSOLE, SEND_TO_MQTT, -1);

    prevTime = millis();

    Serial.println("ESP32 Alarm is ready to use!");
    beep(1, SHORT_BEEP);
}

const unsigned long TEMP_READING_DELAY = 20; // in min
const int ALARM_TIME_DELAY = 3;              // in sec

void loop()
{
    // Gets temperature every so often
    if (millis() - prevTime >= TEMP_READING_DELAY * 60000)
    {
        prevTime = millis();
        checkTempC(LOG_TO_CONSOLE, SEND_TO_MQTT, -1);
    }

    if (settingAlarm && !alarmSet)
    {
        for (size_t i = 0; i < ALARM_TIME_DELAY; i++)
        {
            if (!settingAlarm)
                break; // Exiting the loop if STOP is pressed
            Serial.printf("Alarm on in %d...\n", ALARM_TIME_DELAY - i);
            beep(); // One short beep, this takes 100 ms
            delay(900);
        }
        if (settingAlarm)
        {
            Serial.println("Alarm set!");
            mqttConnect();
            mqttClient.publish("home/esp32/alarm/status", "ALARM ON");
            beep(1, LONG_BEEP);
            settingAlarm = false;
            alarmSet = true;
        }
    }

    if (alarmSet)
    {
        for (int i = 0; i < sizeof(motionSensors) / sizeof(motionSensors[0]); i++)
        {
            motionSensors[i].checkForMovement(LOG_TO_CONSOLE, SEND_TO_MQTT, "home/esp32/alarm/movement/");
        }
    }
}