/* Copyright 2018 Jocelyn Turcotte <turcotte.j@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef USB_RAWHID
#error Teensiduino must be configured to use the "Raw HID" USB Type.
#endif

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIXELS_PIN 23
#define BUTTON_PIN 0

#define RAWHID_RX_SIZE 64
#define NUMPIXELS 24
#define SOURCE_BUF_SIZE NUMPIXELS * 3
#define SUSPEND_LEDS_AFTER 30000

enum MonitorState {
    MONITORING,
    ANIMATING_TO_ZERO,
    SUSPENDED
};

Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
extern const uint8_t gamma8[];

uint8_t recv_buf[RAWHID_RX_SIZE];
uint8_t source_buf1[SOURCE_BUF_SIZE];
uint8_t source_buf2[SOURCE_BUF_SIZE];
uint8_t *current_buf = source_buf1;
uint8_t *prev_buf = source_buf2;
uint32_t prev_buf_swap_millis = 0;
uint32_t buf_swap_millis = 1;
bool bright_when_idle = false;
bool key_was_pressed = false;
// Prevents unwanted events due to a partial contact.
uint32_t button_event_millis = 0;
MonitorState monitor_state = SUSPENDED;

void setup() {
    Pixels.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void swap_buffers(uint32_t now) {
    auto tmp = current_buf;
    current_buf = prev_buf;
    prev_buf = tmp;
    prev_buf_swap_millis = buf_swap_millis;
    buf_swap_millis = now;
}

void render_animation_frame(uint32_t elapsed, uint32_t duration) {
    for (uint16_t i = 0; i < NUMPIXELS; ++i) {
        uint8_t r = ((duration - elapsed) * prev_buf[i*3+0] + elapsed * current_buf[i*3+0]) / duration;
        uint8_t g = ((duration - elapsed) * prev_buf[i*3+1] + elapsed * current_buf[i*3+1]) / duration;
        uint8_t b = ((duration - elapsed) * prev_buf[i*3+2] + elapsed * current_buf[i*3+2]) / duration;
        Pixels.setPixelColor(i, Pixels.Color(
            pgm_read_byte(&gamma8[r]),
            pgm_read_byte(&gamma8[g]),
            pgm_read_byte(&gamma8[b])));
    }

    Pixels.show();
}

void loop() {
    uint32_t now = millis();

    // === Handle the button key press/release
    if (digitalRead(BUTTON_PIN) == LOW && !key_was_pressed && now - button_event_millis > 10) {
        // If suspended, send the remote wake-up ctl.
        if (UDINT & (1<<SUSPI)) {
            UDCON |= (1<<RMWKUP);
            while (UDCON & (1<<RMWKUP));
        }

        // Do one animation step by setting the next buffer state to what we'd expect
        // from receiving stats with that flag.
        if (bright_when_idle) {
            bright_when_idle = false;
            memset(prev_buf, 0, SOURCE_BUF_SIZE);
        } else {
            bright_when_idle = true;
            memset(prev_buf, 255, SOURCE_BUF_SIZE);
        }
        swap_buffers(now);

        key_was_pressed = true;
        button_event_millis = now;
    } else if (digitalRead(BUTTON_PIN) == HIGH && key_was_pressed && now - button_event_millis > 10) {
        key_was_pressed = false;
        button_event_millis = now;
    }

    // === Check if any new pixel info was written on USB
    int r = RawHID.recv(recv_buf, 0);
    if (r > 0) {
        monitor_state = MONITORING;

        // Walk the read buffer to extract each pixel target color during the next interpolation.
        uint8_t *read_ptr = recv_buf;
        for (uint8_t color_offset = 0; color_offset < 3; ++color_offset) {
            // Start with red, then green, then blue
            uint8_t *write_ptr = prev_buf + color_offset;
            // First byte is the number of values for r, g or b.
            uint8_t num_sources = *read_ptr++;
            // Then for each read byte, copy the read value by splitting the ring in enough parts
            uint8_t dupli_count = NUMPIXELS / num_sources;
            uint8_t *end_read = read_ptr + num_sources;
            for (; read_ptr < end_read; ++read_ptr) {
                for (uint8_t pixel = 0; pixel < dupli_count; ++pixel) {
                    *write_ptr = *read_ptr;
                    // Move to the next pixel for this component.
                    write_ptr += 3;
                }
            }
        }

        // "Reverse" the RGB if the lamp is in bright-when-idle mode.
        if (bright_when_idle) {
            for (uint16_t i = 0; i < NUMPIXELS; ++i) {
                uint8_t& r = prev_buf[i*3+0];
                uint8_t& g = prev_buf[i*3+1];
                uint8_t& b = prev_buf[i*3+2];
                uint8_t r2 = min(255, uint16_t(255 - max(g, b)) + r);
                uint8_t g2 = min(255, uint16_t(255 - max(r, b)) + g);
                uint8_t b2 = min(255, uint16_t(255 - max(r, g)) + b);
                r = r2;
                g = g2;
                b = b2;
            }
        }

        swap_buffers(now);
    }

    // === Animate by interpolating between the two last received pixel values
    // Assume that the updates are sent with an even timing.
    // Use the previous duration for the current interpolation.
    const uint32_t duration = buf_swap_millis - prev_buf_swap_millis;
    uint32_t elapsed = now - buf_swap_millis;
    if (elapsed <= duration) {
        render_animation_frame(elapsed, duration);
    } else if (monitor_state == MONITORING && elapsed >= duration * 2) {
        monitor_state = ANIMATING_TO_ZERO;
        memset(prev_buf, 0, SOURCE_BUF_SIZE);
        swap_buffers(now);
    } else if (monitor_state == ANIMATING_TO_ZERO) {
        monitor_state = SUSPENDED;
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
