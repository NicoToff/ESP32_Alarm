/*************** Examples of parametized GET and POST requests *************************/
const char *PARAM_MESSAGE = "message"; // used for tests

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
