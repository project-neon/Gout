/*
*********************************************************************************
GOUT (https://github.com/UFABC-NoBox/Gout)

This code was made by:

  -Marcos Vinícius de Oliveira
  -Renato Fernades Rodrigues
  -Victor Freitas Daga
  -João Pedro Vilas Boas Silva

The GOUT project consists of a flux sensor and a system that help and award the
user over it's water usage. The sensor on the feeder of the water reservoir,
measures the amount of water that enters it, and through a very easy to use and
affordable microcontroller with wireless connection (ESP8266), the system can
read and display to the user the water he used, and compare it to a previously
set goal.

Uploaded and tested at a NODEMCU v0.1 Board with ESP-12E
At arduino software choose: NodeMCU 1.0(ESP-12E Module) at 80MHz, 115200, 4M (3 SPIFFS)

*********************************************************************************
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>            //Library for DNS Server on ESP
#include <ESP8266WebServer.h>     //Library for an ESP WebServer, generally built in!
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
/*
The WiFiManager Library, is the one that enables the ESP8266 to when on connect
to a previously known WiFi, or ask the user to connect the system to his wifi.
*/
#include <WiFiClient.h>           //Makes the ESP Connect to a WiFi
#include <ESP8266mDNS.h>          //Creates an mDNS for the ESP on your local network
ESP8266WebServer server(80);      //Starts the Web Server on port 80 of the ESP8266

//The GPIO on the NODEMCU that the flux sensor is connected to
int flowPin = 4;

//Flow Count Variables
int flow_l_s, flow_l = 0;
long t1, flow_interval = 0;
int flow_l_total=0;
unsigned long flowcount = 0;

// Variable to store time between the pulses
unsigned long t2=0;
// Goal of water to be used, set by the user
int meta = 2000;

//Strings that will receive the website code
String webPage = "";
String webPage_aux = "";

#define countof(a) (sizeof(a) / sizeof(a[0]))

//In this void, the system deals with access to server's root
void handleRoot() {
  server.send(200, "text/html", webPage_aux); //Sending the HTML to the user with code 200
}

//This void handles not found links on the server
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message); // Sending an ERROR 404 and a message
}

//This fuction is called on every interrupt to increment the counter
void flow() {
  flowcount += 1;
}

//Calculating the water flow over time using micros and the data from the function flow
void calc_flow() {

  flow_interval = micros() - t1;
  t1 = micros();

  flow_l_s = flowcount * 100 / 450; // results in 100
  flow_l = flow_l_s * 1000000 / flow_interval;

  Serial.println("litros:" + String(flow_l) + "vel:" + String(flow_l_s) + "\tInterval:" + String(flow_interval) + "\tCounter:" + String(flowcount));
  flowcount =0;
  //store the total flow
  flow_l_total +=flow_l;

}
//On setup we describe the webPage and set connectivity
void setup(void) {
/*
This is the HTML code for the webPage that displays the sensor data on the server
The full code with line ending is at this project GitHub page as index.html
*/
  webPage +="<!DOCTYPE html>";
  webPage +="<html>";
  webPage +="<style>";
  webPage +="body{background-color:#72bf44;}";
  webPage +="h1{color:#FFFFFF; font-family:verdana; text-align:center; font-size:400%;}";
  webPage +="h2{color:#FFFFFF;font-family:verdana;text-align:center;font-size:100%;}";
  webPage +="table {align-self: center; border: 3px solid white; border-collapse: collapse; border-spacing: 10px; }";
  webPage +="td, th {border: 3px solid white;padding: 5px;}";
  webPage +="#head{background-color:#72bf44;border: 2px solid white;position: absolute; top: 1%;width:99%;height: 20%;}";
  webPage +="#master{background-color: #FFFFFF;}";
  webPage +="#sections{background-color:#FFFFFF; border: 2px solid white; padding: 10em; position: absolute;top: 30%; width:80%;  height: 30%; margin-left: 1%;margin-right: 1%; margin-top: inherit;}";
  webPage +="#gastos{background-color:#72bf44;color: white;border-radius: 2em;border: 2px solid white; padding: 3em;position: absolute;top: 20%;left:20%;}";
  webPage +="#metas{ background-color:#72bf44; color: white; border-radius: 2em; border: 2px solid white; padding: 3em;position: absolute;top: 20%;right: 20%;}";
  webPage +="</style> ";
  webPage +="<head><title>GOUT | Economize</title> </head>";
  //webPage +="<body style=\"background-color: #FFFFF;\">";
  webPage +="<div id = \"head\"><h1>GOUT</h1></div>";
  webPage +="<div id=\"sections\">";

  //Define Flow sensor pin and interrupt
  pinMode(flowPin, INPUT_PULLUP);
  attachInterrupt(flowPin, flow, CHANGE);

  //Begins Serial and WiFiManager
  Serial.begin(115200);
  WiFiManager wifiManager;

  //sets timeout until configuration portal gets turned off
  wifiManager.setTimeout(180);

  //reset saved settings
  //wifiManager.resetSettings();

  if (!wifiManager.autoConnect("NoFlow", "87654321")) {   //fetches ssid and pass from eeprom and tries to connect
    Serial.println("failed to connect and hit timeout");  //if it does not connect it starts an access point with the specified name
    delay(3000);                                          //here  "NoFlow"
                                                          //and goes into a blocking loop awaiting configuration
    ESP.reset();                                           //reset and try again, or maybe put it to deep sleep
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //Prints the IP address at the serial connection for debug
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Starts the mDNS server as GOUT
  if (MDNS.begin("GOUT")) {
    Serial.println("MDNS responder started");
  }

  //If someone connects on root, run handle root function
  server.on("/", handleRoot);

  //On NotFound files at the server, runs handleNotFound function
  server.onNotFound(handleNotFound);

  //Begins Server
  server.begin();
  Serial.println("HTTP server started");

}

// At the loop updates the webpage with flow data everytime the user connects
// and count flow every second.
void loop() {

  // The trick used here was finishing the webPage at the loop with the flux data.
  webPage_aux = webPage;
  webPage_aux +="<section id=\"metas\"><h1>Meta:</h1>";
  webPage_aux +="<h2 style=\"align-self:center\">";
  webPage_aux +=flow_l_total;
  webPage_aux +="litros</h2></section>";
  webPage_aux +="<section id=\"gastos\"><h1>Gasto:</h1> <h2>";
  webPage_aux +=meta;
  webPage_aux +="litros</h2></section>";
  webPage_aux +="</div>";
  webPage_aux +="</body>";
  webPage_aux +="</html>";

  //Run calc_flow every 1 second
  if(micros()-t2>1000000){
    calc_flow();
    t2 = micros();
  }

  //Send code 200 (OK) and the hole webPage
  server.send(200, "text/html", webPage_aux);

  //Handles the client connection
  server.handleClient();

}
