/*
 * Codemash by pedroCX486.
 * Last edited on 20180905.
 * Extra credits: u/AdventurersClub, steve0hh
 *
 * "Search for anything out of your wildest dreams and you may understand."
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>

//Your wifi settings
const char* ssid     = "";
const char* password = "";

//Pushbullet Settings
const char* host = "api.pushbullet.com";
const int httpsPort = 443;
const char* accessToken = ""; //These you generate on your pushbullet account control panel
const char* fingerprint = ""; //You can grab the fingerprint of api.pushbullet.com from here: https://www.grc.com/fingerprints.htm

//The strings with content for the push notif. but with generic text
String pushTitle = "Something Went Wrong";
String pushContent = "You shouldn't been seeing this message. (Error 1)";

//Setting our server and port
ESP8266WebServer server(80);

//The HTML to submit text
const char INDEX_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>ESP8266 Web Form</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>ESP8266/NodeMCU Push Notification with Pushbullet</h1>"
"<FORM action=\"/\" method=\"post\">"
"<P>"
"Title:<br>"
"<INPUT type=\"text\" name=\"title\"><BR><BR>"
"Body:<br>"
"<INPUT type=\"text\" name=\"body\"><BR><br>"
"<INPUT type=\"submit\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

//Here we check if it has arguments and how to handle it
void handleRoot()
{
  if (server.hasArg("title") && server.hasArg("body")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

//Error-out in case it has no args
void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

//Handling the submit and the arguments
void handleSubmit()
{
  if (!server.hasArg("title")|| !server.hasArg("body")) return returnFail("BAD ARGS");
  pushTitle = server.arg("title");
  pushContent = server.arg("body");
  Serial.println("Message assembled: " + pushTitle + " " + pushContent);
  sendPush(pushTitle, pushContent);
  Serial.println("done");
  server.send(200, "text/html", INDEX_HTML);
}

//Simple return OK
void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");
}

//Error-out in case it doesn't find the URL requested
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//Sending our push to Pushbullet
void sendPush(String title, String content) {
    
    // Use WiFiClientSecure class to create TLS connection
    WiFiClientSecure client;
    Serial.print("Connecting to ");
    Serial.println(host);
    if (!client.connect(host, httpsPort)) {
      Serial.println("Connection failed!");
      return;
    }

    //Verify if the fingerprint is valid
    if (client.verify(fingerprint, host)) {
      Serial.println("Fingerprint matches!");
    } else {
      Serial.println("Fingerprint mismatch!");
    }

    //The pushbullet API v2 URL
    String url = "/v2/pushes";
    Serial.print("requesting URL: "); Serial.println(url);

    //Our content body
    String contentBody = "{\"body\":\"" + content + "\",\"title\":\"" + title + "\",\"type\":\"note\"}";
	
	//You can use the string bellow if you want to send notifications to a specific channel
    //String contentBody = "{\"body\":\"" + content + "\",\"title\":\"" + title + "\",\"type\":\"note\",\"channel_tag\":\"yourChannelTag\"}";

    //Printing it to pushbullet
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Access-Token: " + accessToken + "\r\n" +
                 "Content-length: "+ contentBody.length() + " \r\n" +
                 "Content-Type: application/json\r\n" +
                 "Accept-Charset: utf-8\r\n" +
                 "Connection: close\r\n\r\n" +
                 contentBody);

    //Printing it to the serial console for debugging reasons
    Serial.println(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Access-Token: " + accessToken + "\r\n" +
                 "Content-length: "+ contentBody.length() + " \r\n" +
                 "Content-Type: application/json\r\n" +
                 "Accept-Charset: utf-8\r\n" +
                 "Connection: close\r\n\r\n" +
                 contentBody);

    //Receive the headers for debugging and to confirm everything went right
    Serial.println("Request sent.");
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("Headers received!");
        break;
      }
    }

    /*
    //This entire area is mostly useful to debug errors in your requests. Enable only if necessary.
    String line = client.readStringUntil('\n');
    //String line = client.readString(); //Enable this to read the entire client response. Useful for debugging.
    if (line.startsWith("{\"active\":true")) {
      Serial.println("ESP8266/Arduino CI successfull!");
    } else {
      Serial.println("ESP8266/Arduino CI has failed");
    }
    Serial.println("Reply was:");
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("Closing connection.");
    */
  }

void setup() {
  Serial.begin(115200);
  delay(10);

  //Connect to your wireless network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  //Start server
  server.begin();
  Serial.println();
  Serial.println("Server started.");
  Serial.print("Connected to ");
  Serial.println(ssid);
 
  Serial.print("Your page is located at: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  server.handleClient();
}
