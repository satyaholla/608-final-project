// #include "SString.h"
// #include <vector>

// typedef unsigned long long bigNum;
// struct personKey {
//     SString person;
//     bigNum prime;
//     bigNum generator;
//     bigNum a;
//     bigNum g_b;
//     bigNum key;
// };

// class KeyExchanger {
//     private:
//         int state;
//         SString this_user;
//         SString other_user;
//         std::vector<personKey> keys;

//         bigNum set_a(const bigNum &p, const bigNum &g, personKey pk) {
//             pk.a = rand() % p;
//             return largePow(p, g, pk.a);
//         }

//         bigNum largePow(const bigNum &p, bigNum g, bigNum a) {
//             bigNum res = 1;
//             while (a) {
//                 if (a % 2) res *= g;

//                 a >>= 1;
//                 g *= g;
//                 g %= p;
//                 res %= p;
//             }
//             return res;
//         }

//     public:
//         KeyExchanger(SString this_user, SString other_user):
//                      this_user(this_user), other_user(other_user), state(0) {
//             state = 0;
//         }

//         /**
//          * @brief updates the key exchanger
//          * 
//          * @param button1 button1 value, 1 if short press and 2 if long press
//          * @param button2 button2 value, 1 if short press and 2 if long press
//          * @param button3 button3 value, 1 if short press and 2 if long press
//          * @param button4 button4 value, 1 if short press and 2 if long press
//          * @param output a char array representing the output to be printed to the tft
//          * @return int 
//          */
//         int update(int button1, int button2, int button3, int button4, char* output) {
//             bigNum g_a;
//             personKey pk;

//             switch (state) {
//                 case 0:
//                     request[0] = '\0';
//                     sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=gen_exchange HTTP/1.1\r\n", user, USERNAME);
//                     strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
//                     strcat(request, "\r\n");
//                     do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//                     {
//                         char response_copy[OUT_BUFFER_SIZE];
//                         strcpy(response_copy, response);
//                         int comma_ind = -1;
//                         while (response_copy[++comma_ind] != ',');
//                         response_copy[comma_ind] = '\0';
//                         pk.prime = atoi(response_copy);
//                         pk.generator = atoi(response_copy + comma_ind + 1);
//                     }

//                     g_a = set_a(pk.prime, pk.generator, pk);

//                     // POST THE G_A
//                     {
//                         char body[200];
//                         sprintf(body, "post_type=%s&other_user=%s&this_user=%s&exponent=%llu", "exp_exchange", user, USERNAME, g_a); //generate body, posting temp, humidity to server
//                         int body_len = strlen(body); //calculate body length (for header reporting)
//                         sprintf(request, "POST http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py HTTP/1.1\r\n");
//                         strcat(request, "Host: 608dev-2.net\r\n");
//                         strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
//                         sprintf(request + strlen(request), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
//                         strcat(request, "\r\n"); //new line from header to body
//                         strcat(request, body); //body
//                         strcat(request, "\r\n"); //new line
//                         do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//                     }

//                     state = 1;
//                     key_exchange_timer = millis();
//                     break;
//                 case 1:
//                     if (millis() - key_exchange_timer < KEY_EXCHANGE_TIMEOUT) {
//                         if (millis() - last_get_request > 100) {
//                         request[0] = '\0';
//                         sprintf(request, "GET http://608dev-2.net/sandbox/sc/sgholla/final-project/key_exchange.py?other_user=%s&this_user=%s&get_type=exp_exchange HTTP/1.1\r\n", user, USERNAME);
//                         strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
//                         strcat(request, "\r\n");
//                         do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
//                         if (response[0] != 'x') {
//                             Serial.println("REACHED HERE");
//                             pk.g_b = atoi(response);
//                             state = 2;
//                         } else {
//                             Serial.println("Response is not yet i guesss");
//                         }
//                         last_get_request = millis();
//                         }
//                     }
//                     else {
//                         Serial.println("Exponent exchange failed, trying again.");
//                         state = 0;
//                     }
//                     break;
//                 case 2:
//                     pk.key = largePow(pk.prime, pk.g_b, pk.a);
//                     Serial.println("Done");
//                     state = 3;
//                     break;
//                     case 3:
//                     keys.push_back(pk);
//                     break;
//             }

//         }
// };