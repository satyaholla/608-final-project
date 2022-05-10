#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "Button.h"
#include "SString.h"
#include <random>
#include <vector>

using namespace std;



  const char USERNAME[] = "shruthi";

  typedef unsigned long long bigNum;

  TFT_eSPI tft = TFT_eSPI();

  bool printed = false;

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


  const uint8_t BUTTON1 = 34;
  const uint8_t BUTTON2 = 39;
  const uint8_t BUTTON3 = 45;
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

  void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
    if (client2.connect(host, 80)) { //try to connect to host on port 80
      if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
      client2.print(request);
      memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
      uint32_t count = millis();
      while (client2.connected()) { //while we remain connected read out data coming back
        client2.readBytesUntil('\n',response,response_size);
        if (serial) Serial.println(response);
        if (strcmp(response,"\r")==0) { //found a blank line!
          break;
        }
        memset(response, 0, response_size);
        if (millis()-count>response_timeout) break;
      }
      memset(response, 0, response_size);  
      count = millis();
      while (client2.available()) { //read out remaining text (body of response)
        char_append(response,client2.read(), OUT_BUFFER_SIZE);
      }
      if (serial) Serial.println(response);
      client2.stop();
      if (serial) Serial.println("-----------");  
    }else{
      if (serial) Serial.println("connection failed :/");
      if (serial) Serial.println("wait 0.5 sec...");
      client2.stop();
    }
  }  

  uint32_t timer;

  struct personKey {
    SString person;
    bigNum prime;
    bigNum generator;
    bigNum a;
    bigNum g_b;
    bigNum key;
  };

  std::vector<SString> new_messages;
  std::vector<personKey> keys;

  SString tft_output = "";
  int user_select_state = 0;
  SString user_to_send_message;
  int main_state = 0;

  class KeyExchanger {
    private:
    enum KeyExchangerState { GET_P_AND_G, WAIT, POST_GA, RECEIVE_GB, CALCULATE_KEY, REST };
    KeyExchangerState state;
    const char* this_user;
    const char* other_user;
    uint32_t key_exchange_timer;
    uint32_t last_get_request;
    uint32_t wait_timer;
    const uint16_t KEY_EXCHANGE_TIMEOUT = 10000;
    const uint16_t GET_REQUEST_PERIOD = 100;
    const uint16_t WAIT_TIME = 500;
    personKey pk;
    

    bigNum set_a(const bigNum &p, const bigNum &g, personKey pk) {
        pk.a = rand() % p;
        return largePow(p, g, pk.a);
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

    public:
    KeyExchanger(const char* this_user, const char* other_user):
                  this_user(this_user), other_user(other_user), state(GET_P_AND_G) {
        last_get_request = millis();
    }

    /**
     * @brief updates the key exchanger
     * 
     * @param button1 button1 value, 1 if short press and 2 if long press
     * @param button2 button2 value, 1 if short press and 2 if long press
     * @param button3 button3 value, 1 if short press and 2 if long press
     * @param button4 button4 value, 1 if short press and 2 if long press
     * @param output a char array representing the output to be printed to the tft
     * @return int 
     */
    int update(/*int button1, int button2, int button3, int button4, char* output*/) {
        bigNum g_a;
        

        switch (state) {
            case GET_P_AND_G:
                request[0] = '\0';
                sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=gen_exchange HTTP/1.1\r\n", other_user, this_user);
                strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
                strcat(request, "\r\n");
                do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
                if (response[0] != '^') {
                    SString response_copy_string = response;
                    char* response_copy = response_copy_string.rawCharArray();
                    int comma_ind = -1;
                    while (response_copy[++comma_ind] != ',');
                    response_copy[comma_ind] = '\0';
                    pk.prime = atoi(response_copy);
                    pk.generator = atoi(response_copy + comma_ind + 1);
                }
                else {
                  state = WAIT;
                  wait_timer = millis();
                }
                state = POST_GA;
                break;
            case WAIT:
                if (millis() - wait_timer > WAIT_TIME) {
                  state = GET_P_AND_G;
                }
                break;
            case POST_GA:
                g_a = set_a(pk.prime, pk.generator, pk);

                // POST THE G_A
                {
                    char body[200];
                    sprintf(body, "post_type=%s&other_user=%s&this_user=%s&exponent=%llu", "exp_exchange", other_user, this_user, g_a); //generate body, posting temp, humidity to server
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

                state = RECEIVE_GB;
                key_exchange_timer = millis();
                break;
            case RECEIVE_GB:
                if (millis() - key_exchange_timer < KEY_EXCHANGE_TIMEOUT) {
                    if (millis() - last_get_request > GET_REQUEST_PERIOD) {
                      request[0] = '\0';
                      sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=exp_exchange HTTP/1.1\r\n", other_user, this_user);
                      strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
                      strcat(request, "\r\n");
                      do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
                    if (response[0] != '^') {
                        Serial.println("REACHED HERE");
                        pk.g_b = atoi(response);
                        state = CALCULATE_KEY;
                    } else {
                        Serial.println("Response is not yet i guesss");
                    }
                    last_get_request = millis();
                    }
                }
                else {
                    Serial.println("Exponent exchange failed.");
                    return -1;
                    state = GET_P_AND_G;
                }
                break;
            case CALCULATE_KEY:
                pk.key = largePow(pk.prime, pk.g_b, pk.a);
                Serial.println("Done");
                state = REST;
                keys.push_back(pk);
                return 1;
                break;
            case REST:
                return 2;
                break;
        }
        return 0;
    }
  };

  class MessageHandler {
    private:
    const uint16_t MESSAGE_HANDLING_PERIOD = 500;
    uint32_t last_message_check;
    const char* this_user;

    static char shifted_char(int c, int shift) {
      int total_num_chars = 126 - 32 + 1;
      c -= 32;
      c += total_num_chars + shift;
      c %= total_num_chars;
      return c + 32;
    }

    public:
    MessageHandler(const char* this_user): this_user(this_user) { last_message_check = millis(); }

    /**
     * @brief checks for messages
     * 
     * @return int 
     */
    int update() {
      if (millis() - last_message_check > MESSAGE_HANDLING_PERIOD) {
        request[0] = '\0';
        sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/message.py?recipient=%s HTTP/1.1\r\n", this_user);
        strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
        strcat(request, "\r\n");
        do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        Serial.println(response);
        if (response[0] != '^') {
          new_messages.push_back(SString(response));
        }
        last_message_check = millis();
      }
      return 0;
    }

    static SString& handle_encryption(SString text, const long long &key, bool encrypt=true) {
      int offset = key % 26;
      SString newString = text;
      for (int i = 0; i < newString.getLength(); i++) {
        newString.rawCharArray()[i] = shifted_char(newString.rawCharArray()[i], offset*(encrypt? 1: -1));
      }
      return newString;
    }
  };

  class NewKeyExchangeHandler {
    private:
    const uint16_t KEY_EXCHANGE_CHECK_PERIOD = 500;
    uint32_t last_key_exchange_check;
    std::vector<KeyExchanger> exchangers;
    const char* this_user;

    public:
    NewKeyExchangeHandler(const char* this_user): this_user(this_user) { last_key_exchange_check = millis(); }

    /**
     * @brief checks for new key exchanges
     * 
     * @return int 
     */
    int update() {
      if (millis() - last_key_exchange_check > KEY_EXCHANGE_CHECK_PERIOD) {
        request[0] = '\0';
        sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?recipient=%s&get_type=new_key_exchange HTTP/1.1\r\n", this_user);
        strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
        strcat(request, "\r\n");
        do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        Serial.println(response);
        if (response[0] != '^') {
          exchangers.push_back(KeyExchanger(this_user, SString(response).rawCharArray()));
        }
        last_key_exchange_check = millis();
      }
      for (int i = exchangers.size() - 1; i >= 0; i--) {
        int result = exchangers[i].update();
        if (result == -1) { exchangers.erase(exchangers.begin() + i); }
      }
      return 0;
    }
  };

  Button button1(34);
  Button button2(39);
  Button button3(45);
  NewKeyExchangeHandler key_exchange_handler(USERNAME);
  MessageHandler message_handler(USERNAME);
  KeyExchanger* key_exchanger_pointer = nullptr;


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
  pinMode(BUTTON3, INPUT_PULLUP);

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
  srand(millis());
}

uint32_t m_time;

//main body of code
void loop() {
  uint32_t M_DELAY = 3000;
  int b1 = button1.update();
  int b2 = button2.update();
  int b3 = button3.update();

  
  key_exchange_handler.update();
  message_handler.update();

  tft.setCursor(0,0,1);
  long long key = -1;
  bool found_user;
  switch (main_state) {
    case 0:
      tft_output = "Welcome. Please select a user to send a message \n\n";
      switch(user_select_state) {
        case 0: user_to_send_message = "ezahid"; break;
        case 1: user_to_send_message = "sgholla"; break;
        default: user_to_send_message = "bruh";
      }
      tft_output = tft_output + user_to_send_message;
      if (b1 == 1) { user_select_state = 1 - user_select_state; tft.fillScreen(TFT_BLACK); }
      if (b1 == 2) { tft.fillScreen(TFT_BLACK); main_state = 1; }
      if (b2 == 1) { tft.fillScreen(TFT_BLACK); main_state = 5; }
    break;
    case 1:
      found_user = false;
      for (int i = 0; i < keys.size(); i++) {
        if (keys[i].person == user_to_send_message) {
          found_user = true;
          key = keys[i].key;
          main_state = 3;
          break;
        }
      }
      if (!found_user) {
        main_state = 2;
        key_exchanger_pointer = new KeyExchanger(USERNAME, user_to_send_message.rawCharArray());
        m_time = millis();
      }
    break;
    case 2:
      // don't have key yet
      tft_output = "Doing key exchange";
      Serial.println("Doing key exchange");
      switch (key_exchanger_pointer->update()) {
        case -1:
          delete key_exchange_pointer;
          key_exchanger_pointer = nullptr;
          main_state = 0;
          break;
        case 1:
          delete key_exchange_pointer;
          key_exchanger_pointer = nullptr;
          main_state = 1;
          break;
      };
      
    break;
    case 3:
      tft_output = SString("Sending \"yo what's up\" to ") + user_to_send_message;
      send_message(user_to_send_message.rawCharArray(), "yo what's up", key);
      main_state = 0;
    break;
    case 5:
      tft_output = "Here are your current messages.\n";
      for (int i = 0; i < new_messages.size(); i++) {
        tft_output = tft_output + new_messages[i] + "\n";
      }
      new_messages.clear();
      if (b2 == 1) { tft.fillScreen(TFT_BLACK); main_state = 0; }
  }  
  tft.println(tft_output.rawCharArray());

  // button_state = digitalRead(BUTTON1);
  // other_button_state = digitalRead(BUTTON2);
  // if (button_state) { key_exchange_state = 0; }
  // if (other_button_state) { message_send_state = 0; }

  // if (!button_state) {
  //   printed = false;
  //   key_exchange("ezahid");
  // }
  // else if (!other_button_state) {
  //   send_message("ezahid", "What's up bro");
  // }
  // else if (!printed && prime != -1) {
  //   delay(1000);
  //   Serial.println("Final Values:\n");
  //   Serial.println(prime);
  //   Serial.println(generator);
  //   Serial.println(a);
  //   Serial.println(g_b);
  //   Serial.println(key);

  //   printed = true;
  // }
}

// TODO: make a standard get/post requesting function

void send_message(const char* user, const char* message, bigNum key) {
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
  message_send_state = 1;
}

// void message_handler() {
//   if (millis() - last_message_check > MESSAGE_HANDLING_TIMEOUT) {
//     request[0] = '\0';
//     sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/message.py?recipient=%s HTTP/1.1\r\n", USERNAME);
//     strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
//     strcat(request, "\r\n");
//     do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//     Serial.println(response);
//     if (response[0] != '^') {
//       new_messages.push_back(SString(response));
//     }
//     last_message_check = millis();
//   }
// }

// void key_exchange(const char* user) { // TODO: make this a class update function
//   bigNum g_a;
//   personKey pk;

//   switch (key_exchange_state) {
//     case 0:
//       request[0] = '\0';
//       sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=gen_exchange HTTP/1.1\r\n", user, USERNAME);
//       strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
//       strcat(request, "\r\n");
//       do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//       {
//         char response_copy[OUT_BUFFER_SIZE];
//         strcpy(response_copy, response);
//         int comma_ind = -1;
//         while (response_copy[++comma_ind] != ',');
//         response_copy[comma_ind] = '\0';
//         pk.prime = atoi(response_copy);
//         pk.generator = atoi(response_copy + comma_ind + 1);
//       }

//       g_a = set_a(pk.prime, pk.generator, pk);

//       // POST THE G_A
//       {
//         char body[200];
//         sprintf(body, "post_type=%s&other_user=%s&this_user=%s&exponent=%llu", "exp_exchange", user, USERNAME, g_a); //generate body, posting temp, humidity to server
//         int body_len = strlen(body); //calculate body length (for header reporting)
//         sprintf(request, "POST http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py HTTP/1.1\r\n");
//         strcat(request, "Host: 608dev-2.net\r\n");
//         strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
//         sprintf(request + strlen(request), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//         strcat(request, "\r\n"); //new line from header to body
//         strcat(request, body); //body
//         strcat(request, "\r\n"); //new line
//         do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//       }

//       key_exchange_state = 1;
//       key_exchange_timer = millis();
//       break;
//     case 1:
//       if (millis() - key_exchange_timer < KEY_EXCHANGE_TIMEOUT) {
//         if (millis() - last_get_request > 100) {
//           request[0] = '\0';
//           sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=exp_exchange HTTP/1.1\r\n", user, USERNAME);
//           strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
//           strcat(request, "\r\n");
//           do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//           if (response[0] != 'x') {
//             Serial.println("REACHED HERE");
//             pk.g_b = atoi(response);
//             key_exchange_state = 2;
//           } else {
//             Serial.println("Response is not yet i guesss");
//           }
//           last_get_request = millis();
//         }
//       }
//       else {
//         Serial.println("Exponent exchange failed, trying again.");
//         key_exchange_state = 0;
//       }
//       break;
//     case 2:
//       pk.key = largePow(pk.prime, pk.g_b, pk.a);
//       Serial.println("Done");
//       key_exchange_state = 3;
//       break;
//     case 3:
//       keys.push_back(pk);
//       break;
//   }

// }

// bigNum set_a(const bigNum &p, const bigNum &g, personKey pk) {
//   pk.a = rand() % p;
//   return largePow(p, g, pk.a);
// }

// bigNum largePow(const bigNum &p, bigNum g, bigNum a) {
//   bigNum res = 1;
//   while (a) {
//     if (a % 2) res *= g;

//     a >>= 1;
//     g *= g;
//     g %= p;
//     res %= p;
//   }
//   return res;
// }