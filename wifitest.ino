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
const int runningLed = 2; // used to visually confirm the program is running
const int max_alerts = 2; // number of alerts before we trip a breaker and stop alerting
const unsigned long max_wait = 80000; // number of miliseconds 

// variables for sensors
int water1State = 0; // var for reading plate sensor #1
int prior_w1state = 0; // placeholder for comparing state changes for sensor #1
// vars for timers and logic settings
int flash_counter = 0; // how count through loop() for flashing the led
int alerts = 0; // how many alerts have been sent
int silent = 0; // 0 = not silent , 1 = silent mode
unsigned long breaker_hit_ms = 0; // store miliseconds uptime when we hit circuit breaker


void setup() {
  // Water sensor setup 
  // initialize the LED pin as an output:
  pinMode(floodLed, OUTPUT);
  digitalWrite(floodLed, HIGH);
  // init water plate as a input button
  pinMode(water1, INPUT);
  
  Serial.begin(115200);
  delay(100);

  // We start by connecting to a WiFi network
  pinMode(runningLed, OUTPUT);
  digitalWrite(runningLed, LOW);
  setup_wifi();
  digitalWrite(runningLed, HIGH);
}



// main loop
void loop() {
  // send your sensor data
  check_water1(); // this will call send_ifttt & sendsensor for ifttt and flask post
  flash(); // intermittently flash the blue led so we know it's still working
  if (circuit_breaker() == 1) {
    silent = 1;
  }
} // end main loop


// Subroutines below here
// ----------------------

int circuit_breaker() {
  unsigned long current_ms = millis();
  Serial.print("current_ms:");
  Serial.println(current_ms);
  Serial.print("breaker_hit_ms:");
  Serial.print(breaker_hit_ms);
  delay(1000);
  if (current_ms - breaker_hit_ms >= max_wait) {
    Serial.println("----------->resetting the circuit breaker.");
    delay(1000);
    silent = 0;
    alerts = 0;
    breaker_hit_ms = current_ms;
    return(0);
  }
  Serial.println("You have sent: " + String(alerts) + " alerts.");
  if (alerts > max_alerts) {
     Serial.println("Alerts hit breaker max" + String(max_alerts) + " going silent for " + String(max_wait) + "ms");
     if (silent == 0) {
        breaker_hit_ms = millis(); // record uptime when we hit the circuit breaker
     }
     return(1);
  } else {
    return(0);
  }
} //end breaker

void flash() {
  /* alternate the blue light so we know we haven't crashed
   * this is better than a delay since we don't miss a sensor check
   * last if block should be the sum of the two other if blocks to make
   * even blink of on/off
  */
  if (flash_counter < 200) {
    digitalWrite(runningLed, HIGH);
  } 
  if (flash_counter > 200) {
    digitalWrite(runningLed, LOW);
  } 
  flash_counter++;
  if (flash_counter == 400) {
    flash_counter = 0;
  }
} // end flash routine

void setup_wifi() {
  // connect to SSID from config.h
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

void sendsensor(int status) {
  if (silent == 1) {
    Serial.println("skipping flask send due to circuit breaker");
    return;
  }
  Serial.print("you sent in a status of: " + String(status));
  delay(1000);
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
  client.print(String("GET ") + url + devicename + "/water1/" + String(status) + " HTTP/1.1\r\n" +
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
  prior_w1state = water1State;
  water1State = digitalRead(water1); // read water sensor #1
  if (prior_w1state == water1State) {
    Serial.println("Same state as before:" + String(water1State));
    return;
  }
  // check if wet, because plate would be high #dank
  if (water1State == HIGH) {
    //turn LED on:
    digitalWrite(floodLed, LOW); // not sure why LOW is on for feather pin #0
    Serial.println("plate wet: light should go ON");
    delay(500);
    send_ifttt("wet");
    delay(500);
    sendsensor(1); // call out to flask 0=dry 1=wet
  } else {
    // turn LED off:
    digitalWrite(floodLed, HIGH);
    Serial.println("plate dry: light should go OFF");
    sendsensor(0); // call out to flask 0=dry 1=wet
    send_ifttt("dry");
    delay(500);
  }
}

void send_ifttt(String status) {
  if (silent == 1) {
    Serial.println("skipping send due to circuit breaker");
    return;
  }
  // here is where you should do some logic
  // like read a sensor to send to the server
  alerts++;
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
  
  // This will send the HTTP request to the server
  String full_url = (String("GET ") + url + ifttt_key + "?value1=" + status + " HTTP/1.1\r\n" +
               "Host: " + ifttt + "\r\n" + 
               "Connection: close\r\n\r\n");
  Serial.println("trying: " + full_url);
  client.print(full_url);

  delay(500);
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    // Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection on IFTTT ------------");
}
