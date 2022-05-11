#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>
#include<math.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

char network[] = "MIT";
char password[] = "";
/* Having network issues since there are 50 MIT and MIT_GUEST networks?. Do the following:
    When the access points are printed out at the start, find a particularly strong one that you're targeting.
    Let's say it is an MIT one and it has the following entry:
   . 4: MIT, Ch:1 (-51dBm)  4:95:E6:AE:DB:41
   Do the following...set the variable channel below to be the channel shown (1 in this example)
   and then copy the MAC address into the byte array below like shown.  Note the values are rendered in hexadecimal
   That is specified by putting a leading 0x in front of the number. We need to specify six pairs of hex values so:
   a 4 turns into a 0x04 (put a leading 0 if only one printed)
   a 95 becomes a 0x95, etc...
   see starting values below that match the example above. Change for your use:
   Finally where you connect to the network, comment out
     WiFi.begin(network, password);
   and uncomment out:
     WiFi.begin(network, password, channel, bssid);
   This will allow you target a specific router rather than a random one!
*/
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.

const uint8_t LOOP_PERIOD = 1000; //milliseconds
uint32_t primary_timer = 0;
uint32_t posting_timer = 0;
uint32_t step_timer = 0;
float x, y, z; //variables for grabbing x,y,and z values
int speed_dial = 0;

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

const uint8_t BUTTON1 = 45; //pin connected to button
const uint8_t BUTTON2 = 39; //pin connected to button
const uint8_t BUTTON3 = 38;
const uint8_t BUTTON4 = 34;

//some suggested variables you can use or delete:

uint8_t message_state; //state of posting

void setup() {
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)
  Wire.begin();
  delay(50); //pause to make sure comms get set up


  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
      uint8_t* cc = WiFi.BSSID(i);
      for (int k = 0; k < 6; k++) {
        Serial.print(*cc, HEX);
        if (k != 5) Serial.print(":");
        cc++;
      }
      Serial.println("");
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);

  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  pinMode(BUTTON1, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON2, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON3, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON4, INPUT_PULLUP);

  message_state = 0; //initialize to something!
}


void loop() {

  //get button readings:
  uint8_t button1 = digitalRead(BUTTON1);
  uint8_t button2 = digitalRead(BUTTON2);
  uint8_t button3 = digitalRead(BUTTON3);
  uint8_t button4 = digitalRead(BUTTON4);

  message_fsm(button1, button2, button3); //run post_reporter_fsm (written here)

  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}


//Post reporting state machine, uses button1 as input
//use post_state for your state variable!
void message_fsm(uint8_t button1, uint8_t button2, uint8_t button3) {
  switch(message_state){
    case 0: //welcome screen
      if (button1 == 0){
        message_state = 1;
        Serial.println("button1 pressed");  
      }
      if (button2 == 0){
        message_state = 2;
        Serial.println("button2 pressed");
      }
      if (button3 == 0){
        message_state = 3;
        Serial.println("button3 pressed");
      }
      break;
    case 1: //send hey jeremy it's kyle
      if (button1 == 1){
        /*POST REQUEST TO UPDATE DATABASE TO MOST RECENT DATA*/
        char body[1000]; //for body
        sprintf(body,"sender=%s&recipient=%s&message=%s", "kyle", "jeremy", "hey kyle here"); //generate body
        //Serial.println("made it here");
        int body_len = strlen(body); //calculate body length (for header reporting)
        sprintf(request_buffer,"POST https://608dev-2.net/sandbox/sc/shruthi/final/message.py HTTP/1.1\r\n");
        //Serial.println("issue here");
        strcat(request_buffer,"Host: 608dev-2.net\r\n");
        strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        //Serial.println("keep going");
        strcat(request_buffer,"\r\n"); //new line from header to body
        strcat(request_buffer,body); //body
        strcat(request_buffer,"\r\n"); //new line
        //Serial.println(request_buffer);
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,false);
        Serial.println(response_buffer);
        }
        Serial.println("finished post req 2");d
  
      message_state = 0;
      break;
    case 2:
    Serial.println("in case 2");
      if (button2 == 1){
        Serial.println("doing post request");
      /*POST REQUEST TO UPDATE DATABASE TO MOST RECENT DATA*/
        char body[1000]; //for body
        sprintf(body,"sender=%s&recipient=%s&message=%s", "kyle", "jeremy", "yo jeremy what up buddy"); //generate body
        //Serial.println("made it here");
        int body_len = strlen(body); //calculate body length (for header reporting)
        sprintf(request_buffer,"POST https://608dev-2.net/sandbox/sc/shruthi/final/message.py HTTP/1.1\r\n");
        //Serial.println("issue here");
        strcat(request_buffer,"Host: 608dev-2.net\r\n");
        strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        //Serial.println("keep going");
        strcat(request_buffer,"\r\n"); //new line from header to body
        strcat(request_buffer,body); //body
        strcat(request_buffer,"\r\n"); //new line
        //Serial.println(request_buffer);
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,false);
        Serial.println(response_buffer);
        }
        Serial.println("finished post req 2");
       
      message_state = 0;
      break;
    case 3:
      if(button3 == 1){
        Serial.println("doing get request");
        sprintf(request_buffer,"GET https://608dev-2.net/sandbox/sc/shruthi/final/message.py/?recipient=jeremy HTTP/1.1\r\n");
        strcat(request_buffer,"Host: 608dev-2.net\r\n"); //add more to the end
        strcat(request_buffer,"\r\n"); //add blank line!
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        Serial.println(response_buffer);
        tft.println(response_buffer);
      }
      message_state = 0;
      break;
    
  }
}

