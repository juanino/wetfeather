/*
 *  Simple HTTP get webclient test
 */

#include <ESP8266WiFi.h>
#include "config.h"

/* Adjust your secrets in config.h 
   ssdid, password, hostname or ip */

// fixed items
const char* devicename = "laundry1";
const int water1 = 14; // water sensor plate on pin 14
const int floodLed = 0; // builtin led on feather for alarm

// variables
int water1State = 0; // var for reading plate sensor #1

void setup() {
  // Water sensor setup 
  // initialize the LED pin as an output:
  pinMode(floodLed, OUTPUT);
  // init water plate as a input button
  pinMode(water1, INPUT);
  
  Serial.begin(115200);
  delay(100);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int value = 0;

void loop() {
  // send your sensor data
  check_water1();
 /* sendsensor();

  // now checkin
  delay(5000);
  ++value;

  Serial.print("checkin connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 5000;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/checkin/";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + devicename + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  */
}

void sendsensor() {
  // here is where you should do some logic
  // like read a sensor to send to the server
  Serial.print("sendsensor: connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 5000;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/post/";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + devicename + "/water1/1 " + "HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  delay(500);
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection on SENDSENSOR------------");
}

void check_water1() {
  Serial.println("Checking water1 plate sensor.");
  water1State = digitalRead(water1); // read water sensor #1
  // check if wet, because plate would be high #dank
  if (water1State == HIGH) {
    //turn LED on:
    digitalWrite(floodLed, LOW); // not sure why LOW is on for feather pin #0
    Serial.println("plate wet: light should go ON");
    delay(500);
    send_ifttt();
    delay(500);
    sendsensor();
  } else {
    // turn LED off:
    digitalWrite(floodLed, HIGH);
    Serial.println("plate dry: light should go OFF");
    delay(500);
  }
}

void send_ifttt() {
  // here is where you should do some logic
  // like read a sensor to send to the server
  Serial.print("sendsensor: connecting to ");
  Serial.println(ifttt);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(ifttt, 80)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/trigger/water1/with/key/";
  Serial.print("Requesting URL: ");
  Serial.println(url + "SECRET KEY"); // key is ifttt_key from config.h
  
  // This will send the request to the server
  Serial.println(String("GET ") + url + "SECRET KEY" + " HTTP/1.1\r\n" +
               "Host: " + ifttt + "\r\n" + 
               "Connection: close\r\n\r\n");

  client.print(String("GET ") + url + ifttt_key + " HTTP/1.1\r\n" +
               "Host: " + ifttt + "\r\n" + 
               "Connection: close\r\n\r\n");

  delay(500);
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    // Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection on IFTTT ------------");
}
