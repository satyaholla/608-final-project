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

const uint8_t LOOP_PERIOD = 10; //milliseconds
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
MPU6050 imu; //imu object called, appropriately, imu

//some suggested variables you can use or delete:

uint8_t ui_state; //state of posting

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
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

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

  ui_state = 0; //initialize to something!
}


void loop() {
  //get IMU information:


  //Serial.println(x);
  //Serial.println(y);
  //Serial.println(acc_mag);

  //get button readings:
  uint8_t button1 = digitalRead(BUTTON1);
  uint8_t button2 = digitalRead(BUTTON2);
  uint8_t button3 = digitalRead(BUTTON3);
  uint8_t button4 = digitalRead(BUTTON4);

  ui_fsm(button1, button2, button3, button4); //run post_reporter_fsm (written here)

  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}


//Post reporting state machine, uses button1 as input
//use post_state for your state variable!
void ui_fsm(uint8_t button1, uint8_t button2, uint8_t button3, uint8_t button4) {
  switch(ui_state){
    case 0: //welcome screen
      if (button1 == 0){
        ui_state = 1;
        Serial.println("button1 pressed");  
      }
      /*else if (button3 == 1){
        //tft.println("Welcome to home screen.");
        ui_state = 0;
      }*/
      break;
    case 1: //new user info entering
      if (button1 == 1){
        //Serial.println("button1 lifted");
        //tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 2);
        tft.println("Welcome New User! Please enter your info");
        if(button2 == 0){
          ui_state = 2;
          tft.fillScreen(TFT_BLACK);
        }
      }
      break;
    case 2:
      if (button2 == 1){
        tft.setCursor(0,0,2);
        tft.println("Welcome to Audio Graffiti!               ");
        tft.println("Record? Press 1.");
        tft.println("Speed Dial? Press 3");
        tft.println("Find User? Press 4.");
        if(button1 == 0){
          ui_state = 3;
          tft.fillScreen(TFT_BLACK);
          delay(5000);
        }
        else if (button3 == 0){
        ui_state = 5;
        tft.fillScreen(TFT_BLACK);
      }
      else if (button4 == 0){
        ui_state = 12;
        Serial.println("send to 12");
        tft.fillScreen(TFT_BLACK);
      }
      }
      
      /*else if (button3 == 0){
        ui_state = 
      }*/
      break;
    case 3:
      if(button1 == 1){
        tft.setCursor(0,0,2);
        tft.println("Recorded!      ");
        tft.println("Listen? Press 2.   ");
        tft.println("Rerecord? Press 3.");
        if(button2 == 0){
          ui_state = 4;
          tft.fillScreen(TFT_BLACK);
          Serial.println("sending to 4");
        }
        if(button3 == 0){
          ui_state = 2;
          Serial.println("sending to 2");
          tft.fillScreen(TFT_BLACK);
        }
      break;
      }
    case 4:
      if(button2 == 1){
        tft.setCursor(0,0,2);
        tft.println("Playing grafitti!");
        tft.println("Press 3 to return home.");
        if(button3 == 0){
          ui_state = 1;
          tft.fillScreen(TFT_BLACK);
        }
      }
      break;
    
    case 5:
      Serial.println("in 5");
      if(button3 == 1){
        if(button1 == 0){
          ui_state = 6;
        }
        else if(button2 == 0){
          ui_state = 7;
        }
      }
      break;
    case 6:
      Serial.println("in 6");
      if(button1 == 1){
        speed_dial++;
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0,0,2);
        tft.print("User ");
        tft.println(speed_dial % 4);
        ui_state = 5;
      }
/*      tft.setCursor(0,0,2);
      tft.println("pick this user? 1.");
      tft.println("pick another user? 2.");
      if(button4 == 0){
        ui_state = 5;
      }
      else if (button1 == 0){
        ui_state = 7;
        
      }*/
      break;
    case 7:
    if (button2 == 1){
      tft.setCursor(0,0,2);
      tft.print("User ");
      tft.print(speed_dial%4);
      tft.println(" chosen");

      delay(3000);
      ui_state = 8;
    }    
    break;
    case 8:
    tft.setCursor(0,0,2);
    tft.println("Press 1 to record your message.    ");
    //delay(5000);
    if (button1 == 0){
      ui_state = 9;
      tft.fillScreen(TFT_BLACK);
    }
    break;
    case 9:
    if(button1 == 1){
      tft.setCursor(0,0,2);
      tft.println("Recorded!");
      tft.println("Listen? Press 2.   ");
      tft.println("Rerecord? Press 3.");
      if(button2 == 0){
        tft.fillScreen(TFT_BLACK);
          ui_state = 10;
          //tft.fillScreen(TFT_BLACK);
        }
      if(button3 == 0){
        ui_state = 8;
          tft.fillScreen(TFT_BLACK);
        }
    }
    break;
    case 10:
    if(button2 == 1){
      tft.setCursor(0,0,2);
      tft.println("Message is playing");
      tft.println("Press 1 to send");
      if(button1 == 0){
        ui_state = 11;
      }
    }
    break;
    case 11:
    if(button1 == 1){
      tft.setCursor(0,0,2);
      tft.println("Message sent!      ");
      tft.println("Press 3 to return home");
      if(button3 == 0){
        ui_state = 0;
        tft.fillScreen(TFT_BLACK);
      }
    }
    break;
    case 12:
    if(button4 == 1){
      tft.setCursor(0,0,2);
      tft.println("Manually enter user");
      tft.println("press 2 when done");
      if(button2 == 0){
        ui_state = 8;
      }
    }
    break;

  }
}

