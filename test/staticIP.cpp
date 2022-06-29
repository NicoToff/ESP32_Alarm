// Check this: https://electropeak.com/learn/create-a-web-server-w-esp32/
// Try and read an output state with digitalRead()
// Check that: https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AsyncTCP.h>

#define WIFI_NETWORK "NnN"
#define WIFI_PASSWORD "Peketparty89"
#define WIFI_TIMEOUT_MS 10000

WebServer server(80);

// HTML & CSS contents which display on web server
String HTML = R"rawliteral(<!doctype html>
<html lang='en'>
<head>
    <title>Title</title>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>
    <link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/bootstrap.min.css'
        integrity='sha384-0evHe/X+R7YkIZDRvuzKMRqM+OrBnVFBL6DOitfPri4tjfHxaWutUpFmBp4vmVor' crossorigin='anonymous'>
</head>
<body>
    <h1>Bootstrap added</h1>
    <script src='https://cdn.jsdelivr.net/npm/@popperjs/core@2.11.5/dist/umd/popper.min.js'
        integrity='sha384-Xe+8cL9oJa6tN/veChSP7q+mnSPaj5Bcu9mPX5F5xIGE0DVittaqT5lorf0EI7Vk'
        crossorigin='anonymous'></script>
    <script src='https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap.min.js'
        integrity='sha384-kjU+l4N0Yf4ZOJErLsIcvOU2qSb74wXpOhqTvwVx3OElZRweTnQ6d31fXEoRD1Jy'
        crossorigin='anonymous'></script>
</body>
</html>
)rawliteral";

// Setting Static IP
IPAddress local_IP(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// Handle root url (/)
void handle_root()
{
    server.send(200, "text/html", HTML);
}

void setup()
{
    Serial.begin(115200);

    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
        Serial.println("Static IP Failed to configure");
    }

    Serial.println("Try Connecting to ");
    Serial.println(WIFI_NETWORK);

    // Connect to your wi-fi modem
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

    // Check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected successfully");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP()); // Show ESP32 IP on serial

    server.on("/", handle_root);

    server.begin();
    Serial.println("HTTP server started");
    delay(100);
}

void loop()
{
    server.handleClient();
}