#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN            2

#define NUMPIXELS      12

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include "Keyboard.h"
#include "my_usb_api.h"
// #include "Serial.h"

// set pin numbers for the five buttons:
const int upButton = 3;
const int downButton = 4;
const int leftButton = 5;
const int rightButton = 6;
const int mouseButton = 7;

int delayval = 10; // delay for half a second

uint8_t rawhid_buffer[64];


void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
// #if defined (__AVR_ATtiny85__)
//   if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
// #endif
  // End of trinket special code

  // pixels.begin(); // This initializes the NeoPixel library.

  // pinMode(upButton, INPUT_PULLUP);
  // pinMode(downButton, INPUT_PULLUP);
  // pinMode(leftButton, INPUT_PULLUP);
  // pinMode(rightButton, INPUT_PULLUP);
  // pinMode(mouseButton, INPUT_PULLUP);

  // Keyboard.begin();
  Serial.begin(9600);
}

int i = 0;
int r = 0;
int g = 0;
int b = 0;
int vals[3] = {0,0,0};
int cur = 0;
void loop() {

  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
    // Serial.println("Hello World...");
    delay(1000);
  
    r = RawHID.recv(rawhid_buffer, 0);
    if (r > 0) {
      digitalWrite(11, HIGH);
      delay(500);
      digitalWrite(11, LOW);
      // for(int i=0;i<NUMPIXELS;i++){
      //   pixels.setPixelColor(i, pixels.Color(rawhid_buffer[i*3+0], rawhid_buffer[i*3+1], rawhid_buffer[i*3+2])); // Moderately bright green color.
      // }

      // pixels.show(); // This sends the updated pixel color to the hardware.
    }


    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255

    // delay(delayval); // Delay for a period of time (in milliseconds).

//  }
  // i++;
  // if (i > NUMPIXELS) {
  //   i = 0;
  //   vals[cur] += 16;
  //   if (vals[cur] > 128) {
  //     vals[cur] = 0;
  //     cur = (cur + 1) % 3;
  //   }
  // }

  // // use the pushbuttons to control the keyboard:
  // if (digitalRead(upButton) == LOW) {
  //   Keyboard.write('u');
  // }
  // if (digitalRead(downButton) == LOW) {
  //   Keyboard.write('d');
  // }
  // if (digitalRead(leftButton) == LOW) {
  //   Keyboard.write('l');
  // }
  // if (digitalRead(rightButton) == LOW) {
  //   Keyboard.write('r');
  // }
  // if (digitalRead(mouseButton) == LOW) {
  //   Keyboard.write('m');
  // }
}
