/*
 *  Using adafruit esp8266 feather
 *  More info at https://github.com/juanino/wetfeather
 */

#include <ESP8266WiFi.h>
#include "config.h"

/* Adjust your secrets in config.h 
   ssdid, password, hostname or ip */

// fixed items
const char* devicename = "laundry1";
const int water1 = 14; // water sensor plate on pin 14
const int water2 = 12; // seond water sensor on pin 12
const int floodLed = 0; // builtin RED led on feather for alarm
const int runningLed = 2; // BLUE led used to visually confirm the program is running
const int max_alerts = 4; // number of alerts before we trip a breaker and stop alerting
const unsigned long max_wait = 900000; // number of miliseconds before the breaker is reset

// variables for sensors
int water1State = 0; // var for reading plate sensor #1
int water2State = 0; //second sensor
int prior_w1state = 0; // placeholder for comparing state changes for sensor #1
int prior_w2state = 0; // second sensor
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
  pinMode(water2, INPUT);
  
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
  /* check sensor
   * call send_ifttt for push msg
   * call sendsensor flask post and SMS 
   */
     
  /* check each sensor */
  check_water(water1, &prior_w1state, &water1State);
  check_water(water2, &prior_w2state, &water2State);
   
  /* intermittently flash the blue led so we know it's still working */
  flash(); 

  /* check if too many alerts */
  if (circuit_breaker() == 1) {
    int prior_silent_state = silent;
    silent = 1; // if circuit_breaker() is tripped we go silent for a while
    Serial.println("---------------------");
    Serial.print(String(prior_silent_state) + " " + String(silent));
    if (prior_silent_state != silent) {
      // state change to break on occured
      String msg = "breaker" + String(silent);
      send_ifttt(msg);
      delay(1000);
    }
  }
} // end main loop



// Subroutines below here
// ----------------------

void check_water(int sensorpin, int* prior_state, int* waterXstate ) {
  Serial.println("Checking plate sensor on " + String(sensorpin));
  *prior_state = *waterXstate;
  *waterXstate = digitalRead(sensorpin); // read water sensor #1
  if (*prior_state == *waterXstate) {
    Serial.println("Same state as before:" + String(*waterXstate));
    return;
  }
  // check if wet, because plate would be high #dank
  if (*waterXstate == HIGH) {
    //turn LED on:
    digitalWrite(floodLed, LOW); // not sure why LOW is on for feather pin #0
    Serial.println("plate wet: light should go ON");
    delay(500);
    String wet_msg = String(sensorpin) + "wet";
    send_ifttt(wet_msg);
    delay(500);
    sendsensor(1,sensorpin); // call out to flask 0=dry 1=wet
  } else {
    // turn LED off:
    digitalWrite(floodLed, HIGH);
    Serial.println("plate dry: light should go OFF");
    sendsensor(0,sensorpin); // call out to flask 0=dry 1=wet
    String dry_msg = String(sensorpin) + "dry";
    send_ifttt(dry_msg);
    delay(500);
  }
}


int circuit_breaker() {
  unsigned long current_ms = millis();
  Serial.print("current_ms:");
  Serial.println(current_ms);
  Serial.print("breaker_hit_ms:");
  Serial.print(breaker_hit_ms);
  Serial.println();
  if (current_ms - breaker_hit_ms >= max_wait) {
    Serial.println("----------->resetting the circuit breaker.");
    delay(1000);
    int prior_silent = silent;
    silent = 0;
    if (prior_silent != silent) {
      String msg = "breaker0";
      send_ifttt(msg);
    }
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

void sendsensor(int status, int sensorpin) {
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
  client.print(String("GET ") + url + devicename + "/" + sensorpin + "/" + String(status) + " HTTP/1.1\r\n" +
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



void send_ifttt(String status) {
  Serial.println("Status is" + status);
  delay(1000);
  if (silent == 1 && status != "breaker1" && status != "breaker0") {
    // let it through if it's a breaker message so we know
    // we tripped the breaker
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
