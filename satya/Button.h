#include <SPI.h>
// #include <TFT_eSPI.h>
// TFT_eSPI tft = TFT_eSPI();

//enum for button states
enum button_state {S0,S1,S2,S3,S4};
 
class Button{
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
 
// Button button(BUTTON_PIN);
 
// void setup() {
//   Serial.begin(115200);               // Set up serial port
//   pinMode(BUTTON_PIN, INPUT_PULLUP);
//   tft.init();
//   tft.setRotation(2);
//   tft.setTextSize(1);
//   tft.fillScreen(TFT_BLACK);

// }
 
// //delays are used here just for testing purposes!
// // do not use delays in production
// void loop() {
//   tft.setCursor(0, 15,1);
//   uint8_t flag = button.update();
//   Serial.println(flag);
//   if (flag==1) {
//     tft.print("short press");
//     delay(1000);  //bad practice...just for testing!
//   } else if (flag==2) {
//     tft.print("long press");
//     delay(1000); //bad practice...just for testing!
//   } else {
//     char temp_message[30];
//     sprintf(temp_message,"%u          ",button.state); //for clearing out other messages
//     tft.print(temp_message);
//     delay(1);  //bad practice...just for testing.
//   }
// }