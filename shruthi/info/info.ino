#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>
#include<math.h>
#include <WiFiClientSecure.h>
//WiFiClientSecure is a big library. It can take a bit of time to do that first compile

const char* CA_CERT = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n" \
                      "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n" \
                      "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n" \
                      "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n" \
                      "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n" \
                      "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n" \
                      "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n" \
                      "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n" \
                      "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n" \
                      "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n" \
                      "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n" \
                      "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n" \
                      "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n" \
                      "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n" \
                      "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n" \
                      "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n" \
                      "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n" \
                      "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n" \
                      "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n" \
                      "-----END CERTIFICATE-----\n";

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// user input file

  //enum for button states
  enum button_state {S0, S1, S2, S3, S4};

char username[30]= "";
char pw[30] = "";
int token;
char message[100] = "";
char signup[10] = "signup";
char login[10] = "login";
char function[10];
char attempt_response[50]="";
uint32_t stall_message_timer;
const uint16_t MESSAGE_STALL_PERIOD = 5000;

  class Button {
    public:
    uint32_t S2_start_time;
    uint32_t button_change_time;    
    uint32_t debounce_duration;
    uint32_t long_press_duration;
    uint8_t pin;
    uint8_t flag;
    uint8_t button_pressed;
    button_state state; // This is public for the sake of convenience

    Button(int p) {
      flag = 0;  
      state = S0;
      pin = p;
      S2_start_time = millis(); //init
      button_change_time = millis(); //init
      debounce_duration = 10;
      long_press_duration = 1000;
      button_pressed = 0;
    }

    void read() {
      uint8_t button_val = digitalRead(pin);  
      button_pressed = !button_val; //invert button
    }

    int update() {
      read();
      long long cur_time = millis();
      flag = 0;
      switch (state) {
        case S0:
          if (button_pressed) {
            state = S1;
            button_change_time = cur_time;
          }
          break;
        case S1:
          if (!button_pressed) {
            button_change_time = cur_time;
            state = S0;
          } else if (millis() - button_change_time > debounce_duration) {
            state = S2;
            S2_start_time = millis();
          }
          break;
        case S2:
          if (!button_pressed) {
            state = S4;
            button_change_time = cur_time;
          } else if (cur_time - S2_start_time > long_press_duration) {
            state = S3;
          }
          break;
        case S3:
          if (!button_pressed) {
            state = S4;
            button_change_time = cur_time;
          }
          break;
        case S4:
          if (button_pressed) {
            state = cur_time - S2_start_time > long_press_duration? S3: S2;
            button_change_time = cur_time;
          } else if (cur_time - button_change_time > debounce_duration) {
            flag = button_change_time - S2_start_time > long_press_duration? 2: 1;
            state = S0;
          }
          break;  
        default: Serial.println("Something is messed up.");
      }
      return flag;
    }
  };

  class UserInput {
      char alphabet[50] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
      char encrypted_msg[1000] = {0};
      char msg[400] = {0}; //contains previous query response
      
      char encrypted_query_string[200] = {0};
      char keyword[11];
      int char_index;
      int state;
      uint32_t scroll_timer;
      const int scroll_threshold = 150;
      const float angle_threshold = 0.3;

    public:
      char input_string[50] = {0};
      
      UserInput() {
        state = 0;
        memset(msg, 0, sizeof(msg));//empty it.
        strcat(msg, "           ");
        char_index = 0;
        scroll_timer = millis();
      }

      int update(float angle, int button, char* output, char* change) { // returns current state
        output[0] = '\0';
        uint32_t current_time = millis();
        switch (state) {
          case 0:
            strcat(output, msg);
            if (button == 2) {
              scroll_timer = current_time;
              state = 1;
              char_index = 0;
              //tft.fillScreen(TFT_BLACK);
            }
            break;
            
          case 1:
            if (current_time - scroll_timer > scroll_threshold && button != 1) {
              scroll_timer = current_time;
              if (angle >= angle_threshold) char_index++;
              else if (-angle >= angle_threshold) char_index--;
              char_index += 37;
              char_index %= 37;
            }
            if (button == 2) {
              state = 2;
            }
            else {
              char cur_letter[2];
              cur_letter[0] = alphabet[char_index];
              cur_letter[1] = '\0';
              
              strcat(output, input_string);
              strcat(output, cur_letter);
              if (button == 1) {
                strcat(input_string, cur_letter);
                char_index = 0;
              }
            }
            break;
          
          case 2:
            strcat(output, "Saved!");
            state = 3;
            break;
            
          case 3:
            //Serial.print("inp string is ");
            //Serial.println(input_string);
            //Serial.print("username is empty ");
            //Serial.println(username);
            strcat(change, input_string); 
            //Serial.println("sprintfhappened");
            Serial.print("username ");
            Serial.println(username);

            Serial.print("password ");
            Serial.println(pw);

            


  
            state = 0;
            // strcat(output, msg);
            // Serial.println(msg);
            break;
        }
        return state;
      }
  };


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

const int DELAY = 1000;
const int SAMPLE_FREQ = 8000;                          // Hz, telephone sample rate
const int SAMPLE_DURATION = 5;                        // duration of fixed sampling (seconds)
const int NUM_SAMPLES = SAMPLE_FREQ * SAMPLE_DURATION;  // number of of samples
const int ENC_LEN = (NUM_SAMPLES + 2 - ((NUM_SAMPLES + 2) % 3)) / 3 * 4;  // Encoded length of clip

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char transcript[100] = {0};

const uint8_t BUTTON1 = 45; //pin connected to button
const uint8_t BUTTON2 = 39; //pin connected to button
const uint8_t BUTTON3 = 38;
const uint8_t BUTTON4 = 34;
MPU6050 imu; //imu object called, appropriately, imu

/* CONSTANTS */
//Prefix to POST request:
const char PREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\",\"speechContexts\": [{\"phrases\": [\"Satya\",\"Michelle\"],\"boost\": 10}]}, \"audio\": {\"content\":\"";
const char SUFFIX[] = "\"}}"; //suffix to POST request
const int AUDIO_IN = 1; //pin where microphone is connected
const char API_KEY[] = "AIzaSyAQ9SzqkHhV-Gjv-71LohsypXUH447GWX8"; //don't change this

const uint8_t PIN_1 = 34; //button 1
const uint8_t PIN_2 = 39; //button 2

//some suggested variables you can use or delete:

uint8_t ui_state; //state of posting

/* Global variables*/
uint8_t button_state; //used for containing button state and detecting edges
int old_button_state; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing

char speech_data[ENC_LEN + 200] = {0}; //global used for collecting speech data
const char* NETWORK  = "EECS_Labs";     // your network SSID (name of wifi network)
const char* PASSWORD = ""; // your network password
const char*  SERVER = "speech.google.com";  // Server URL



uint8_t old_val;
uint32_t timer;

WiFiClientSecure client; //global WiFiClient Secure object


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
  //set up lcd
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  ledcSetup(2, 200, 8);
  ledcSetup(3, 200, 8);
  ledcSetup(4, 200, 8);
  
  ledcAttachPin(2, 2);
  ledcAttachPin(3, 3);
  ledcAttachPin(4, 4);

  ledcWrite(2, 255);
  ledcWrite(3, 255);
  ledcWrite(4, 255);
  //set up button
  pinMode(BUTTON1, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON2, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON3, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON4, INPUT_PULLUP);

  timer = millis();
  client.setCACert(CA_CERT); //set cert for https
  old_val = digitalRead(PIN_1);

  ui_state = 0; //initialize to something!
}

UserInput imu_input;
UserInput ii;
Button button_for_input(38);
Button bfi(34);

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

  //get_message();
  //delay(5000);
  
  //tft.fillScreen(TFT_BLACK);
  ui_fsm(button1, button2, button3, button4); //run post_reporter_fsm (written here)

  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}


//Post reporting state machine, uses button1 as input
//use post_state for your state variable!
void ui_fsm(uint8_t button1, uint8_t button2, uint8_t button3, uint8_t button4) {
  switch(ui_state){
    case 0: //welcome screen
      //memset username and password
        username[0]='\0';
        pw[0]='\0';
        get_message();
        delay(5000);
        Serial.println("moved past get_messsage");
      
        ledcWrite(2, 250);
        ledcWrite(3, 250);
        ledcWrite(4, 250);
      tft.setCursor(0, 0, 2);
      tft.println("Welcome!");
      tft.println("Signup? Press 1.");
      tft.println("Login? Press 3.");      
      if (button1 == 0){ //signup
        
        ui_state = 1;
        Serial.println("sent to signup");  
        tft.fillScreen(TFT_BLACK);
        strcat(function,signup);
        
      }
    if (button3 == 0){ //login
        ui_state = 1;
        Serial.println("sent to login");  
        tft.fillScreen(TFT_BLACK);
        strcat(function,login);
      }
  
      break;
    case 1: //sign up
      if (button1 == 1){ 
        //Serial.println("button1 lifted");
        //tft.fillScreen(TFT_BLACK);
        ledcWrite(2, 250);
        ledcWrite(3, 250);
        ledcWrite(4, 250);
        //tft.setCursor(0, 0, 2);
        //tft.println("Login or Signup?");

        float x, y;
        get_angle(&x, &y); //get angle values
        char user_input_array[30];
        //Serial.println("before");
        int imu_input_state = imu_input.update(y, button_for_input.update(), user_input_array, username);
        //Serial.println("after");
        tft.setCursor(0,0,2);
        tft.println(user_input_array);

        
        //if (imu_input_state == 3) {
        //  Serial.println("in state");
          
          //tft.fillScreen(TFT_BLACK);
        //}  
      
        if (button4 == 0){
          Serial.println("button 4 pressed");
          ui_state = 14;
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
          //delay(5000);
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
        //Serial.println("reached case 3");
        ledcWrite(2, 0);
        ledcWrite(3, 250);
        ledcWrite(4, 250);
        transcribe();
        tft.setCursor(0,0,2);
        tft.println("Start? Press 4.                      ");
        tft.println("Review? Press 2.   ");
        tft.println("Restart? Press 3.");
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
        tft.print("Your message is ");
        tft.println(transcript);
        tft.println("Press 3 to return home.");
        ledcWrite(2, 250);
        ledcWrite(3, 0);
        ledcWrite(4, 250);
        if(button3 == 0){
          ui_state = 1;
          tft.fillScreen(TFT_BLACK);
        }
      }
      break;
    
    case 5:
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
      transcribe();
      ledcWrite(2, 0);
      ledcWrite(3, 250);
      ledcWrite(4, 250);
      tft.println("Start? Press 4.         ");
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
      tft.print("Your message is ");
      tft.println(transcript);
      tft.println("Press 1 to send");
      ledcWrite(2, 250);
      ledcWrite(3, 250);
      ledcWrite(4, 0);
      if(button1 == 0){
        ui_state = 11;
      }
    }
    break;
    case 11:
    if(button1 == 1){
      tft.setCursor(0,0,2);
      ledcWrite(2, 250);
        ledcWrite(3, 0);
        ledcWrite(4, 250);
      tft.println("Message sent!       ");
      tft.println("Press 3 to return home.               ");
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
    /*
    case 13: //login
    if (button4 == 1){ 
        //Serial.println("button1 lifted");
        //tft.fillScreen(TFT_BLACK);
        ledcWrite(2, 250);
        ledcWrite(3, 250);
        ledcWrite(4, 250);
        //tft.setCursor(0, 0, 2);
        //tft.println("Login or Signup?");

        float x, y;
        get_angle(&x, &y); //get angle values
        char user_input_array2[30];
        //Serial.println("before");
        int imu_input_state = imu_input.update(y, button_for_input.update(), user_input_array2, username2);
        //Serial.println("after");
        tft.setCursor(0,0,2);
        tft.println(user_input_array2);

        
        //if (imu_input_state == 3) {
        //  Serial.println("in state");
          
          //tft.fillScreen(TFT_BLACK);
        //}  
      
        if (button4 == 0){
          Serial.println("button 4 pressed");
          ui_state = 17;
          tft.fillScreen(TFT_BLACK);
        }

      }
      break;
      */
  case 14:
      if(button4 == 1){
        Serial.println("in case 14");
        ui_state = 15;
        
        

      }
      break;
      case 15:
      {
          float a, b;
          get_angle(&a, &b); //get angle values
          char pw_array[30];
          int iis2 = ii.update(b, bfi.update(), pw_array, pw);
          tft.setCursor(0,0,2);
          tft.println(pw_array);

        if(button2 == 0){
          ui_state = 16;
        }}
        break;
      case 16:
        {      
        Serial.println("in state 16");
        post_method(username, pw, function);
        Serial.println("post request done");
        int val=atoi(attempt_response);
        if(val==0){
          tft.println(attempt_response);
        }
        else{
          tft.println("Successful Login");
          token=atoi(attempt_response);
        }
        stall_message_timer = millis();
        ui_state=17;

        }
        break;
      case 17:
        if (millis() - stall_message_timer > MESSAGE_STALL_PERIOD) {
          ui_state = 18;
        }
        break;
      case 18:
        int val=atoi(attempt_response);
        if(val==0){
          ui_state=0;
        }
        else{
          ui_state=2;
        }
        attempt_response[0]='\0'; //might need to change here (memset instead)
        tft.fillScreen(TFT_BLACK);
        break;

      
    

  }
}


void get_message(){
  Serial.println("get request");
  sprintf(request_buffer,"GET https://608dev-2.net/sandbox/sc/team33/final/messages.py?recipient=arduina&token=12345 HTTP/1.1\r\n");
  strcat(request_buffer,"Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request_buffer,"\r\n"); //add blank line!
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  Serial.println("got a response");
  Serial.println(response_buffer);
  //Serial.printf("%d\n", strcmp(response_buffer, '^'));
  // char temp[5];
  // temp[0] = '\0';
  // strncpy(response_buffer, temp);
  // temp[4] = '\0';
  if (response_buffer[0] != '^'){ //not empty
    Serial.println("message not empty!");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0,0,2);
    tft.println(response_buffer);
  }
  
  

}
void transcribe(){
  button_state = digitalRead(PIN_1);
        //Serial.println(button_state);
  if (!button_state && button_state != old_button_state) {
    Serial.println("listening...");
    record_audio();
    Serial.println("sending...");
    Serial.print("\nStarting connection to server...");
    delay(300);
    bool conn = false;
    for (int i = 0; i < 10; i++) {
      int val = (int)client.connect(SERVER, 443, 4000);
      Serial.print(i); Serial.print(": "); Serial.println(val);
      if (val != 0) {
        conn = true;
        break;
      }
      Serial.print(".");
      delay(300);
    }
    if (!conn) {
      Serial.println("Connection failed!");
      return;
    } else {
      Serial.println("Connected to server!");
      Serial.println(client.connected());
      int len = strlen(speech_data);
      // Make a HTTP request:
      client.print("POST /v1/speech:recognize?key="); client.print(API_KEY); client.print(" HTTP/1.1\r\n");
      client.print("Host: speech.googleapis.com\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("cache-control: no-cache\r\n");
      client.print("Content-Length: "); client.print(len);
      client.print("\r\n\r\n");
      int ind = 0;
      int jump_size = 1000;
      char temp_holder[jump_size + 10] = {0};
      Serial.println("sending data");
      while (ind < len) {
        delay(80);//experiment with this number!
        //if (ind + jump_size < len) client.print(speech_data.substring(ind, ind + jump_size));
        strncat(temp_holder, speech_data + ind, jump_size);
        client.print(temp_holder);
        ind += jump_size;
        memset(temp_holder, 0, sizeof(temp_holder));
      }
      client.print("\r\n");
      //Serial.print("\r\n\r\n");
      Serial.println("Through send...");
      unsigned long count = millis();
      while (client.connected()) {
        Serial.println("IN!");
        String line = client.readStringUntil('\n');
        Serial.print(line);
        if (line == "\r") { //got header of response
          Serial.println("headers received");
          break;
        }
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println("");
      Serial.println("Response...");
      count = millis();
      while (!client.available()) {
        delay(100);
        Serial.print(".");
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println();
      Serial.println("-----------");
      memset(response, 0, sizeof(response));
      while (client.available()) {
        char_append(response, client.read(), OUT_BUFFER_SIZE);
      }
      //Serial.println(response); //comment this out if needed for debugging
      char* trans_id = strstr(response, "transcript");
      if (trans_id != NULL) {
        char* foll_coll = strstr(trans_id, ":");
        char* starto = foll_coll + 2; //starting index
        char* endo = strstr(starto + 1, "\""); //ending index
        int transcript_len = endo - starto + 1;
        transcript[0] = {'\0'};
        strncat(transcript, starto, transcript_len);
        Serial.println(transcript);
      }
      Serial.println("-----------");
      client.stop();
      Serial.println("done");
    }
  }
  old_button_state = button_state;
}
//function used to record audio at sample rate for a fixed nmber of samples
void record_audio() {
  int sample_num = 0;    // counter for samples
  int enc_index = strlen(PREFIX) - 1;  // index counter for encoded samples
  float time_between_samples = 1000000 / SAMPLE_FREQ;
  int value = 0;
  char raw_samples[3];   // 8-bit raw sample data array
  memset(speech_data, 0, sizeof(speech_data));
  sprintf(speech_data, "%s", PREFIX);
  char holder[5] = {0};
  Serial.println("starting");
  uint32_t text_index = enc_index;
  uint32_t start = millis();
  time_since_sample = micros();
  while (sample_num < NUM_SAMPLES && !digitalRead(PIN_1)) { //read in NUM_SAMPLES worth of audio data
    value = analogRead(AUDIO_IN);  //make measurement
    raw_samples[sample_num % 3] = mulaw_encode(value - 1800); //remove 1.5ishV offset (from 12 bit reading)
    sample_num++;
    if (sample_num % 3 == 0) {
      base64_encode(holder, raw_samples, 3);
      strncat(speech_data + text_index, holder, 4);
      text_index += 4;
    }
    // wait till next time to read
    while (micros() - time_since_sample <= time_between_samples); //wait...
    time_since_sample = micros();
  }
  Serial.println(millis() - start);
  sprintf(speech_data + strlen(speech_data), "%s", SUFFIX);
  Serial.println("out");
}


int8_t mulaw_encode(int16_t sample) {
  const uint16_t MULAW_MAX = 0x1FFF;
  const uint16_t MULAW_BIAS = 33;
  uint16_t mask = 0x1000;
  uint8_t sign = 0;
  uint8_t position = 12;
  uint8_t lsb = 0;
  if (sample < 0)
  {
      sample = -sample;
      sign = 0x80;
  }
  sample += MULAW_BIAS;
  if (sample > MULAW_MAX)
  {
      sample = MULAW_MAX;
  }
  for (; ((sample & mask) != mask && position >= 5); mask >>= 1, position--)
        ;
  lsb = (sample >> (position - 4)) & 0x0f;
  return (~(sign | ((position - 5) << 4) | lsb));
}

void post_method(char* user,char* pword,char* function) {
    char body[1000]; 
    sprintf(body,"user=%s&password=%s&function=%s",user,pword,function);
    int body_len = strlen(body); 
    sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/team33/final/request_handler_login.py/ HTTP/1.1\r\n");
    strcat(request_buffer,"Host: 608dev-2.net\r\n");
    strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); 
    strcat(request_buffer,"\r\n"); 
    strcat(request_buffer,body); 
    strcat(request_buffer,"\r\n"); 
    do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
    Serial.println(response_buffer); 
    strcat(attempt_response, response_buffer);


}

