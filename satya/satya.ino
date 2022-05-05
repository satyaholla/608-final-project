#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <String.h>
#include <random>

const char USERNAME[] = "sgholla";

typedef unsigned long long bigNum;

TFT_eSPI tft = TFT_eSPI();
bigNum prime = -1;
bigNum generator = -1;
bigNum a = -1;
bigNum g_b = -1;
bigNum key = 0;

bool printed = false;
int key_exchange_state = 0;
int message_send_state = 0;

uint32_t key_exchange_timer;
uint32_t last_get_request;
uint32_t last_message_check;
const uint16_t KEY_EXCHANGE_TIMEOUT = 10000;
const uint16_t MESSAGE_HANDLING_TIMEOUT = 5000;

const uint16_t RESPONSE_TIMEOUT = 6000;
const uint16_t IN_BUFFER_SIZE = 3500; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
const uint16_t JSON_BODY_SIZE = 3000;
char request[IN_BUFFER_SIZE];
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char json_body[JSON_BODY_SIZE];

/* CONSTANTS */
//Prefix to POST request:
const char PREFIX[] = "{\"wifiAccessPoints\": ["; //beginning of json body
const char SUFFIX[] = "]}"; //suffix to POST request
const char API_KEY[] = "AIzaSyAQ9SzqkHhV-Gjv-71LohsypXUH447GWX8"; //don't change this and don't share this


const uint8_t BUTTON1 = 45;
const uint8_t BUTTON2 = 39;
const int MAX_APS = 5;

/* Global variables*/
uint8_t button_state; //used for containing button state and detecting edges
uint8_t other_button_state; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing

WiFiClientSecure client; //global WiFiClient Secure object
WiFiClient client2; //global WiFiClient Secure object


//choose one:

//const char NETWORK[] = "18_62";
//const char PASSWORD[] = "";
//
//
const char NETWORK[] = "EECS_Labs";
const char PASSWORD[] = "";

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



/*----------------------------------
   wifi_object_builder: generates a json-compatible entry for use with Google's geolocation API
   Arguments:
    * `char* object_string`: a char pointer to a location that can be used to build a c-string with a fully-contained JSON-compatible entry for one WiFi access point
    *  `uint32_t os_len`: a variable informing the function how much  space is available in the buffer
    * `uint8_t channel`: a value indicating the channel of WiFi operation (1 to 14)
    * `int signal_strength`: the value in dBm of the Access point
    * `uint8_t* mac_address`: a pointer to the six long array of `uint8_t` values that specifies the MAC address for the access point in question.

      Return value:
      the length of the object built. If not entry is written, 
*/
int wifi_object_builder(char* object_string, uint32_t os_len, uint8_t channel, int signal_strength, uint8_t* mac_address) {
  char json_object[500];
  char hex_mac_address[19];
  strcpy(hex_mac_address, "");
  strcpy(json_object, "");
  char temp[4];
  for (int i = 0; i < 6; i++) {
    sprintf(temp, "%02x:", mac_address[i]);
    strcat(hex_mac_address, temp);
  }
  hex_mac_address[17] = char(0);
  
  sprintf(json_object, "{\"macAddress\": \"%s\", \"signalStrength\": %d, \"age\": 0, \"channel\": %d}",
      hex_mac_address, signal_strength,int(channel));
  if (strlen(json_object) > os_len) {
    return 0;
  }
  else {
    strcat(object_string, json_object);
    return strlen(json_object);
  }
}

char* SERVER = "googleapis.com";  // Server URL

uint32_t timer;

void setup() {
  Serial.begin(115200);               // Set up serial port

  //SET UP SCREEN:
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size, change if you want
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_PINK, TFT_BLACK); //set color of font to hot pink foreground, black background

  //SET UP BUTTON:
  delay(100); //wait a bit (100 ms)
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  //PRINT OUT WIFI NETWORKS NEARBY
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
  WiFi.begin(NETWORK, PASSWORD);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);

  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(NETWORK);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  timer = millis();
  key_exchange_timer = millis();
  last_message_check = millis();
  srand(millis());
}

//main body of code
void loop() {
  button_state = digitalRead(BUTTON1);
  other_button_state = digitalRead(BUTTON2);
  message_handler();
  if (button_state) { key_exchange_state = 0; }
  if (other_button_state) { message_send_state = 0; }

  if (!button_state) {
    printed = false;
    key_exchange("ezahid");
  }
  else if (!other_button_state) {
    send_message("ezahid", "What's up bro");
  }
  else if (!printed && prime != -1) {
    delay(1000);
    Serial.println("Final Values:\n");
    Serial.println(prime);
    Serial.println(generator);
    Serial.println(a);
    Serial.println(g_b);
    Serial.println(key);

    printed = true;
  }
}

// TODO: make a standard get/post requesting function

void send_message(const char* user, const char* message) {
  switch (message_send_state) {
    case 0: {
      char body[200];
      sprintf(body, "sender=%s&recipient=%s&message=(%s, %llu)", USERNAME, user, message, key); //generate body, posting temp, humidity to server
      int body_len = strlen(body); //calculate body length (for header reporting)
      sprintf(request, "POST http://608dev-2.net/sandbox/sc/sgholla/final-project/message.py HTTP/1.1\r\n");
      strcat(request, "Host: 608dev-2.net\r\n");
      strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
      sprintf(request + strlen(request), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
      strcat(request, "\r\n"); //new line from header to body
      strcat(request, body); //body
      strcat(request, "\r\n"); //new line
      do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
      message_send_state = 1; }
      break;

    case 1:
      break;
  }
  
}

void message_handler() {
  if (millis() - last_message_check > MESSAGE_HANDLING_TIMEOUT) {
    request[0] = '\0';
    sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/message.py?recipient=%s HTTP/1.1\r\n", USERNAME);
    strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
    strcat(request, "\r\n");
    do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println(response);
    last_message_check = millis();
  }
}

void key_exchange(const char* user) { // TODO: make this a class update function
  bigNum g_a;
  switch (key_exchange_state) {
    case 0:
      request[0] = '\0';
      sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=gen_exchange HTTP/1.1\r\n", user, USERNAME);
      strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
      strcat(request, "\r\n");
      do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
      {
        char response_copy[OUT_BUFFER_SIZE];
        strcpy(response_copy, response);
        int comma_ind = -1;
        while (response_copy[++comma_ind] != ',');
        response_copy[comma_ind] = '\0';
        prime = atoi(response_copy);
        generator = atoi(response_copy + comma_ind + 1);
      }

      g_a = set_a(prime, generator);
      Serial.println(g_a);

      // POST THE G_A
      {
        char body[200];
        sprintf(body, "post_type=%s&other_user=%s&this_user=%s&exponent=%llu", "exp_exchange", user, USERNAME, g_a); //generate body, posting temp, humidity to server
        int body_len = strlen(body); //calculate body length (for header reporting)
        sprintf(request, "POST http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py HTTP/1.1\r\n");
        strcat(request, "Host: 608dev-2.net\r\n");
        strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request + strlen(request), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request, "\r\n"); //new line from header to body
        strcat(request, body); //body
        strcat(request, "\r\n"); //new line
        do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
      }

      key_exchange_state = 1;
      key_exchange_timer = millis();
      break;
    case 1:
      if (millis() - key_exchange_timer < KEY_EXCHANGE_TIMEOUT) {
        if (millis() - last_get_request > 100) {
          request[0] = '\0';
          sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=exp_exchange HTTP/1.1\r\n", user, USERNAME);
          strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
          strcat(request, "\r\n");
          do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
          if (response[0] != 'x') {
            Serial.println("REACHED HERE");
            g_b = atoi(response);
            key_exchange_state = 2;
          } else {
            Serial.println("Response is not yet i guesss");
          }
          last_get_request = millis();
        }
      }
      else {
        Serial.println("Exponent exchange failed, trying again.");
        key_exchange_state = 0;
      }
      break;
    case 2:
      key = largePow(prime, g_b, a);
      Serial.println("Done");
      key_exchange_state = 3;
      break;
    case 3:
      break;
  }

}

bigNum set_a(const bigNum &p, const bigNum &g) {
  a = rand() % p;
  return largePow(p, g, a);
}

bigNum largePow(const bigNum &p, bigNum g, bigNum a) {
  bigNum res = 1;
  while (a) {
    if (a % 2) res *= g;

    a >>= 1;
    g *= g;
    g %= p;
    res %= p;
  }
  return res;
}