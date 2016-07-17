#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

// new
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server(80);


int flowPin = 4;

unsigned long flowcount = 0;


#define countof(a) (sizeof(a) / sizeof(a[0]))


void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound(){
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

void setup(void){
  // put your setup code here, to run once:
  pinMode(flowPin, INPUT_PULLUP);
  attachInterrupt(flowPin, flow, CHANGE);

  Serial.begin(115200);

    WiFiManager wifiManager;

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(180);

    //reset saved settings
    //wifiManager.resetSettings();

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration


    //and goes into a blocking loop awaiting configuration
    if(!wifiManager.autoConnect("NoFlow","87654321")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");


  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("GOUT")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  // MDNS.addService("http","tcp",80);
}

void flow()
{
flowcount +=1;
}

void loop(){
  Serial.print("Flow in Liters: ");  Serial.println(flowcount/450.0);

  //Serial.print(digitalRead(flowPin));

  Serial.print("Flow in pulses: ");  Serial.println(flowcount);


 server.handleClient();

  //webString = "Flow: " + String(flow1count/450.0) + " L"; // Arduino has a hard time with float to string


  delay(500);


}
