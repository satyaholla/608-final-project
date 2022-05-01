#include "extra.h"
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>
#include<math.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

char network[] = "MIT";
char password[] = "";

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response



uint32_t primary_timer = 0;
const uint8_t LOOP_PERIOD = 10;
char ASCII[6];
int ASCII_TO_NUMBER[100];
char CIPHER_GRID[]="N13ACH8TBMO2E5RWPD4F67GI9J0QLKSUZYVX";
char plaintext[]="608Week1";
char key[]="privacy";

void setup() {
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)

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

ASCII[0]='A';
ASCII[1]='D';
ASCII[2]='F';
ASCII[3]='G';
ASCII[4]='V';
ASCII[5]='X';

ASCII_TO_NUMBER[int('A')]=0;
ASCII_TO_NUMBER[int('D')]=1;
ASCII_TO_NUMBER[int('F')]=2;
ASCII_TO_NUMBER[int('G')]=3;
ASCII_TO_NUMBER[int('V')]=4;
ASCII_TO_NUMBER[int('X')]=5;

tft.setCursor(0,0,4);

char encrypted_plaintext[40];
char decrypted_plaintext[40];

encrypt(plaintext,encrypted_plaintext);
Serial.println(encrypted_plaintext);
post_method(encrypted_plaintext);


}


void loop() {
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}

void encrypt(char* query, char* encrypted_query) {
  char intermediate_array[80];
  for(int i=0;i<strlen(query);i++){
    int index;
    for (int j=0;j<36;j++){
      if(CIPHER_GRID[j]==query[i]){
        index=j;
      }
    }
    int first_letter=index/6;
    int second_letter=index%6;
    intermediate_array[2*i]=ASCII[first_letter];
    intermediate_array[2*i+1]=ASCII[second_letter];
  }
  int array_order[20];
  int array_values[20];
  for(int i=0;i<strlen(key);i++){
    array_values[i]=int(key[i]);
  }
  for(int i=0;i<strlen(key);i++){
    int smallest_value=1000;
    int smallest_index=-1;
      for(int j=0;j<strlen(key);j++){
        if(array_values[j]<smallest_value){
          smallest_index=j;
          smallest_value=array_values[j];
        }
    }
    array_order[i]=smallest_index;
    array_values[smallest_index]=1000;
  }
  int counter=0;
  for(int i=0;i<strlen(key);i++){
    int index=array_order[i];
    while(int(intermediate_array[index])>=65 && int(intermediate_array[index])<=91){
      encrypted_query[counter]=intermediate_array[index];
      counter+=1;
      index+=strlen(key);
    }
  }
}

void decrypt(char* query, char* decrypted_query){
    int array_order[20];
    int array_values[20];
    char intermediate_array[80];
  for(int i=0;i<strlen(key);i++){
    array_values[i]=int(key[i]);
  }
  for(int i=0;i<strlen(key);i++){
    int smallest_value=1000;
    int smallest_index=-1;
      for(int j=0;j<strlen(key);j++){
        if(array_values[j]<smallest_value){
          smallest_index=j;
          smallest_value=array_values[j];
        }
    }
    array_order[smallest_index]=i;
    array_values[smallest_index]=1000;
  }
  int counter=0;
  int block_length=strlen(query)/strlen(key);
  for (int i=0;i<block_length;i++){
    for(int j=0;j<strlen(key);j++){
      intermediate_array[counter]=query[array_order[j]*block_length+i];
      counter+=1;
    }
  }

   for(int i=0;i<int(strlen(query)/2);i++){
    int first_letter=int(intermediate_array[2*i]);
    int second_letter=int(intermediate_array[2*i+1]);
    if(first_letter>=65 && first_letter<=91 && second_letter>=65 && second_letter<=91){
      decrypted_query[i]=CIPHER_GRID[6*ASCII_TO_NUMBER[first_letter]+ASCII_TO_NUMBER[second_letter]];
    }
  }
    
}

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (true) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}
void post_method(char* post_value) {
    char body[100]; //for body
    sprintf(body,"encrypted_text=%s",post_value);//generate body, posting to User, 1 step
    int body_len = strlen(body); //calculate body length (for header reporting)
    sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/ezahid/final_project/request_handler_encrypted_text.py, HTTP/1.1\r\n");
    strcat(request_buffer,"Host: ezahid@608dev-2.net\r\n");
    strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request_buffer,"\r\n"); //new line from header to body
    strcat(request_buffer,body); //body
    strcat(request_buffer,"\r\n"); //new line
    Serial.println(request_buffer);
    do_http_request("ezahid@608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
    Serial.println(response_buffer); //viewable in Serial Terminal
}
