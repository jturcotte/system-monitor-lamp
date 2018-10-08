#include "Keyboard.h"
#include "my_usb_api.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIXELS_PIN 1
#define BUTTON_PIN 2
#define BUTTON_KEY KEY_ENTER

#define NUMPIXELS 12

Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
extern const uint8_t gamma8[];

uint8_t source_buf1[64];
uint8_t source_buf2[64];
uint8_t *current_buf = source_buf1;
uint8_t *prev_buf = source_buf2;
uint32_t prev_buf_swap_millis = 0;
uint32_t buf_swap_millis = 1;
bool key_was_pressed = false;
// Prevents unwanted events due to a partial contact.
uint32_t button_event_millis = 0;


void setup() {
    Pixels.begin();
    Keyboard.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    uint32_t now = millis();

    if (digitalRead(BUTTON_PIN) == LOW && !key_was_pressed && now - button_event_millis > 10) {
        Keyboard.press(BUTTON_KEY);
        key_was_pressed = true;
        button_event_millis = now;
    } else if (digitalRead(BUTTON_PIN) == HIGH && key_was_pressed && now - button_event_millis > 10) {
        Keyboard.release(BUTTON_KEY);
        key_was_pressed = false;
        button_event_millis = now;
    }

    int r = RawHID.recv(prev_buf, 0);
    if (r > 0) {
        auto tmp = current_buf;
        current_buf = prev_buf;
        prev_buf = tmp;
        prev_buf_swap_millis = buf_swap_millis;
        buf_swap_millis = now;
    }

    // Assume that the updates are sent with an even timing.
    // Use the previous duration for the current interpolation.
    const uint32_t dur = buf_swap_millis - prev_buf_swap_millis;
    uint32_t elapsed = now - buf_swap_millis;
    if (elapsed <= dur) {
        for (int i = 0; i < NUMPIXELS; ++i) {
            uint8_t r = ((dur - elapsed) * prev_buf[i*3+0] + elapsed * current_buf[i*3+0]) / dur;
            uint8_t g = ((dur - elapsed) * prev_buf[i*3+1] + elapsed * current_buf[i*3+1]) / dur;
            uint8_t b = ((dur - elapsed) * prev_buf[i*3+2] + elapsed * current_buf[i*3+2]) / dur;
            Pixels.setPixelColor(i, Pixels.Color(
                pgm_read_byte(&gamma8[r]),
                pgm_read_byte(&gamma8[g]),
                pgm_read_byte(&gamma8[b])));
        }

        Pixels.show();      
    }
}

// https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
