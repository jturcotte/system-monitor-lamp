/* USB API for Teensy USB Development Board
 * http://www.pjrc.com/teensy/teensyduino.html
 * Copyright (c) 2008 PJRC.COM, LLC
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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "usb_common.h"
#include "usb_api.h"
#include "my_usb_private.h"
#include "my_usb_api.h"
#include "wiring.h"

// Step #1, decode UTF8 to Unicode code points
//
size_t usb_keyboard_class::write(uint8_t c)
{
        if (c < 0x80) {
                // single byte encoded, 0x00 to 0x7F
                utf8_state = 0;
                write_unicode(c);
        } else if (c < 0xC0) {
                // 2nd, 3rd or 4th byte, 0x80 to 0xBF
                c &= 0x3F;
                if (utf8_state == 1) {
                        utf8_state = 0;
                        write_unicode(unicode_wchar | c);
                } else if (utf8_state == 2) {
                        unicode_wchar |= ((uint16_t)c << 6);
                        utf8_state = 1;
                }
        } else if (c < 0xE0) {
                // begin 2 byte sequence, 0xC2 to 0xDF
                // or illegal 2 byte sequence, 0xC0 to 0xC1
                unicode_wchar = (uint16_t)(c & 0x1F) << 6;
                utf8_state = 1;
        } else if (c < 0xF0) {
                // begin 3 byte sequence, 0xE0 to 0xEF
                unicode_wchar = (uint16_t)(c & 0x0F) << 12;
                utf8_state = 2;
        } else {
                // begin 4 byte sequence (not supported), 0xF0 to 0xF4
                // or illegal, 0xF5 to 0xFF
                utf8_state = 255;
        }
        return 1;
}


// Step #2: translate Unicode code point to keystroke sequence
//
KEYCODE_TYPE usb_keyboard_class::unicode_to_keycode(uint16_t cpoint)
{
        // Unicode code points beyond U+FFFF are not supported
        // technically this input should probably be called UCS-2
        if (cpoint < 32) {
                if (cpoint == 10) return KEY_ENTER & KEYCODE_MASK;
                if (cpoint == 11) return KEY_TAB & KEYCODE_MASK;
                return 0;
        }
        if (cpoint < 128) {
                if (sizeof(KEYCODE_TYPE) == 1) {
                        return pgm_read_byte(keycodes_ascii + (cpoint - 0x20));
                } else if (sizeof(KEYCODE_TYPE) == 2) {
                        return pgm_read_word(keycodes_ascii + (cpoint - 0x20));
                }
                return 0;
        }
        #ifdef ISO_8859_1_A0
        if (cpoint <= 0xA0) return 0;
        if (cpoint < 0x100) {
                if (sizeof(KEYCODE_TYPE) == 1) {
                        return pgm_read_byte(keycodes_iso_8859_1 + (cpoint - 0xA0));
                } else if (sizeof(KEYCODE_TYPE) == 2) {
                        return pgm_read_word(keycodes_iso_8859_1 + (cpoint - 0xA0));
                }
                return 0;
        }
        #endif
        //#ifdef UNICODE_20AC
        //if (cpoint == 0x20AC) return UNICODE_20AC & 0x3FFF;
        //#endif
        #ifdef KEYCODE_EXTRA00
        if (cpoint == UNICODE_EXTRA00) return KEYCODE_EXTRA00 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA01
        if (cpoint == UNICODE_EXTRA01) return KEYCODE_EXTRA01 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA02
        if (cpoint == UNICODE_EXTRA02) return KEYCODE_EXTRA02 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA03
        if (cpoint == UNICODE_EXTRA03) return KEYCODE_EXTRA03 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA04
        if (cpoint == UNICODE_EXTRA04) return KEYCODE_EXTRA04 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA05
        if (cpoint == UNICODE_EXTRA05) return KEYCODE_EXTRA05 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA06
        if (cpoint == UNICODE_EXTRA06) return KEYCODE_EXTRA06 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA07
        if (cpoint == UNICODE_EXTRA07) return KEYCODE_EXTRA07 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA08
        if (cpoint == UNICODE_EXTRA08) return KEYCODE_EXTRA08 & 0x3FFF;
        #endif
        #ifdef KEYCODE_EXTRA09
        if (cpoint == UNICODE_EXTRA09) return KEYCODE_EXTRA09 & 0x3FFF;
        #endif
        return 0;
}

// Step #3: execute keystroke sequence
//
void usb_keyboard_class::write_keycode(KEYCODE_TYPE keycode)
{
        if (!keycode) return;
        #ifdef DEADKEYS_MASK
        KEYCODE_TYPE deadkeycode = deadkey_to_keycode(keycode);
        if (deadkeycode) write_key(deadkeycode);
        #endif
        write_key(keycode);
}

KEYCODE_TYPE usb_keyboard_class::deadkey_to_keycode(KEYCODE_TYPE keycode)
{
        #ifdef DEADKEYS_MASK
        keycode &= DEADKEYS_MASK;
        if (keycode == 0) return 0;
        #ifdef ACUTE_ACCENT_BITS
        if (keycode == ACUTE_ACCENT_BITS) return DEADKEY_ACUTE_ACCENT;
        #endif
        #ifdef CEDILLA_BITS
        if (keycode == CEDILLA_BITS) return DEADKEY_CEDILLA;
        #endif
        #ifdef CIRCUMFLEX_BITS
        if (keycode == CIRCUMFLEX_BITS) return DEADKEY_CIRCUMFLEX;
        #endif
        #ifdef DIAERESIS_BITS
        if (keycode == DIAERESIS_BITS) return DEADKEY_DIAERESIS;
        #endif
        #ifdef GRAVE_ACCENT_BITS
        if (keycode == GRAVE_ACCENT_BITS) return DEADKEY_GRAVE_ACCENT;
        #endif
        #ifdef TILDE_BITS
        if (keycode == TILDE_BITS) return DEADKEY_TILDE;
        #endif
        #ifdef RING_ABOVE_BITS
        if (keycode == RING_ABOVE_BITS) return DEADKEY_RING_ABOVE;
        #endif
        #endif // DEADKEYS_MASK
        return 0;
}

// Step #4: do each keystroke
//
void usb_keyboard_class::write_key(KEYCODE_TYPE keycode)
{
        keyboard_report_data[0] = keycode_to_modifier(keycode);
        keyboard_report_data[1] = 0;
        keyboard_report_data[2] = keycode_to_key(keycode);
        keyboard_report_data[3] = 0;
        keyboard_report_data[4] = 0;
        keyboard_report_data[5] = 0;
        keyboard_report_data[6] = 0;
        keyboard_report_data[7] = 0;
        send_now();
        keyboard_report_data[0] = 0;
        keyboard_report_data[2] = 0;
        send_now();
}

uint8_t usb_keyboard_class::keycode_to_modifier(KEYCODE_TYPE keycode)
{
        uint8_t modifier=0;

        #ifdef SHIFT_MASK
        if (keycode & SHIFT_MASK) modifier |= MODIFIERKEY_SHIFT;
        #endif
        #ifdef ALTGR_MASK
        if (keycode & ALTGR_MASK) modifier |= MODIFIERKEY_RIGHT_ALT;
        #endif
        #ifdef RCTRL_MASK
        if (keycode & RCTRL_MASK) modifier |= MODIFIERKEY_RIGHT_CTRL;
        #endif
        return modifier;
}

uint8_t usb_keyboard_class::keycode_to_key(KEYCODE_TYPE keycode)
{
        uint8_t key = keycode & 0x3F;
        #ifdef KEY_NON_US_100
        if (key == KEY_NON_US_100) key = 100;
        #endif
        return key;
}



void usb_keyboard_class::set_modifier(uint16_t c)
{
        keyboard_report_data[0] = (uint8_t)c;
}
void usb_keyboard_class::set_key1(uint8_t c)
{
        keyboard_report_data[2] = c;
}
void usb_keyboard_class::set_key2(uint8_t c)
{
        keyboard_report_data[3] = c;
}
void usb_keyboard_class::set_key3(uint8_t c)
{
        keyboard_report_data[4] = c;
}
void usb_keyboard_class::set_key4(uint8_t c)
{
        keyboard_report_data[5] = c;
}
void usb_keyboard_class::set_key5(uint8_t c)
{
        keyboard_report_data[6] = c;
}
void usb_keyboard_class::set_key6(uint8_t c)
{
        keyboard_report_data[7] = c;
}


void usb_keyboard_class::send_now(void)
{
        uint8_t intr_state, timeout;

        if (!usb_configuration) return;
        intr_state = SREG;
        cli();
        UENUM = KEYBOARD_ENDPOINT;
        timeout = UDFNUML + 50;
        while (1) {
                // are we ready to transmit?
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                // has the USB gone offline?
                if (!usb_configuration) return;
                // have we waited too long?
                if (UDFNUML == timeout) return;
                // get ready to try checking again
                intr_state = SREG;
                cli();
                UENUM = KEYBOARD_ENDPOINT;
        }
        UEDATX = keyboard_report_data[0];
        UEDATX = keyboard_report_data[1];
        UEDATX = keyboard_report_data[2];
        UEDATX = keyboard_report_data[3];
        UEDATX = keyboard_report_data[4];
        UEDATX = keyboard_report_data[5];
        UEDATX = keyboard_report_data[6];
        UEDATX = keyboard_report_data[7];
        UEINTX = 0x3A;
        keyboard_idle_count = 0;
        SREG = intr_state;
}


void usb_keyboard_class::press(uint16_t n)
{
        uint8_t key, mod, msb, modrestore=0;

        msb = n >> 8;
        if (msb >= 0xC2) {
                if (msb <= 0xDF) {
                        n = (n & 0x3F) | ((uint16_t)(msb & 0x1F) << 6);
                } else if (msb == 0xF0) {
                        presskey(n, 0);
                        return;
                } else if (msb == 0xE0) {
                        presskey(0, n);
                        return;
                } else if (msb == 0xE2) {
                        press_system_key(n);
                        return;
                } else if (msb >= 0xE4 && msb <= 0xE7) {
                        press_consumer_key(n & 0x3FF);
                        return;
                } else {
                        return;
                }
        }
        KEYCODE_TYPE keycode = unicode_to_keycode(n);
        if (!keycode) return;
#ifdef DEADKEYS_MASK
        KEYCODE_TYPE deadkeycode = deadkey_to_keycode(keycode);
        if (deadkeycode) {
                modrestore = keyboard_report_data[0];
                if (modrestore) {
                        keyboard_report_data[0] = 0;
                        send_now();
                }
                // TODO: test if operating systems recognize
                // deadkey sequences when other keys are held
                mod = keycode_to_modifier(deadkeycode);
                key = keycode_to_key(deadkeycode);
                presskey(key, mod);
                releasekey(key, mod);
        }
#endif
        mod = keycode_to_modifier(keycode);
        key = keycode_to_key(keycode);
        presskey(key, mod | modrestore);
}

void usb_keyboard_class::release(uint16_t n)
{
        uint8_t key, mod, msb;

        msb = n >> 8;
        if (msb >= 0xC2) {
                if (msb <= 0xDF) {
                        n = (n & 0x3F) | ((uint16_t)(msb & 0x1F) << 6);
                } else if (msb == 0xF0) {
                        releasekey(n, 0);
                        return;
                } else if (msb == 0xE0) {
                        releasekey(0, n);
                        return;
                } else if (msb == 0xE2) {
                        release_system_key(n);
                        return;
                } else if (msb >= 0xE4 && msb <= 0xE7) {
                        release_consumer_key(n & 0x3FF);
                        return;
                } else {
                        return;
                }
        }
        KEYCODE_TYPE keycode = unicode_to_keycode(n);
        if (!keycode) return;
        mod = keycode_to_modifier(keycode);
        key = keycode_to_key(keycode);
        releasekey(key, mod);
}

void usb_keyboard_class::presskey(uint8_t key, uint8_t modifier)
{
        bool send_required = false;
        uint8_t i;

        if (modifier) {
                if ((keyboard_report_data[0] & modifier) != modifier) {
                        keyboard_report_data[0] |= modifier;
                        send_required = true;
                }
        }
        if (key) {
                for (i=2; i < 8; i++) {
                        if (keyboard_report_data[i] == key) goto end;
                }
                for (i=2; i < 8; i++) {
                        if (keyboard_report_data[i] == 0) {
                                keyboard_report_data[i] = key;
                                send_required = true;
                                goto end;
                        }
                }
        }
        end:
        if (send_required) send_now();
}

void usb_keyboard_class::releasekey(uint8_t key, uint8_t modifier)
{
        bool send_required = false;
        uint8_t i;

        if (modifier) {
                if ((keyboard_report_data[0] & modifier) != 0) {
                        keyboard_report_data[0] &= ~modifier;
                        send_required = true;
                }
        }
        if (key) {
                for (i=2; i < 8; i++) {
                        if (keyboard_report_data[i] == key) {
                                keyboard_report_data[i] = 0;
                                send_required = true;
                        }
                }
        }
        if (send_required) send_now();
}

void usb_keyboard_class::releaseAll(void)
{
        uint8_t i, anybits;

        anybits = keyboard_report_data[0];
        for (i=2; i < 8; i++) {
                anybits |= keyboard_report_data[i];
                keyboard_report_data[i] = 0;
        }
        if (!anybits) return;
        keyboard_report_data[0] = 0;
        send_now();
}

void usb_keyboard_class::press_consumer_key(uint16_t key)
{
        if (key == 0) return;
        for (uint8_t i=0; i < 4; i++) {
                if (keymedia_consumer_keys[i] == key) return;
        }
        for (uint8_t i=0; i < 4; i++) {
                if (keymedia_consumer_keys[i] == 0) {
                        keymedia_consumer_keys[i] = key;
                        keymedia_send();
                        return;
                }
        }
}

void usb_keyboard_class::release_consumer_key(uint16_t key)
{
        if (key == 0) return;
        for (uint8_t i=0; i < 4; i++) {
                if (keymedia_consumer_keys[i] == key) {
                        keymedia_consumer_keys[i] = 0;
                        keymedia_send();
                        return;
                }
        }
}

void usb_keyboard_class::press_system_key(uint8_t key)
{
        if (key == 0) return;
        for (uint8_t i=0; i < 3; i++) {
                if (keymedia_system_keys[i] == key) return;
        }
        for (uint8_t i=0; i < 3; i++) {
                if (keymedia_system_keys[i] == 0) {
                        keymedia_system_keys[i] = key;
                        keymedia_send();
                        return;
                }
        }
}

void usb_keyboard_class::release_system_key(uint8_t key)
{
        if (key == 0) return;
        for (uint8_t i=0; i < 3; i++) {
                if (keymedia_system_keys[i] == key) {
                        keymedia_system_keys[i] = 0;
                        keymedia_send();
                        return;
                }
        }
}

void usb_keyboard_class::keymedia_release_all(void)
{
        uint8_t anybits = 0;
        for (uint8_t i=0; i < 4; i++) {
                if (keymedia_consumer_keys[i] != 0) anybits = 1;
                keymedia_consumer_keys[i] = 0;
        }
        for (uint8_t i=0; i < 3; i++) {
                if (keymedia_system_keys[i] != 0) anybits = 1;
                keymedia_system_keys[i] = 0;
        }
        if (anybits) keymedia_send();
}

// send the contents of keyboard_keys and keyboard_modifier_keys
void usb_keyboard_class::keymedia_send(void)
{
        uint8_t intr_state, timeout;

        if (!usb_configuration) return;
        intr_state = SREG;
        cli();
        UENUM = KEYMEDIA_ENDPOINT;
        timeout = UDFNUML + 50;
        while (1) {
                // are we ready to transmit?
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                // has the USB gone offline?
                if (!usb_configuration) return;
                // have we waited too long?
                if (UDFNUML == timeout) return;
                // get ready to try checking again
                intr_state = SREG;
                cli();
                UENUM = KEYMEDIA_ENDPOINT;
        }
        // 44444444 44333333 33332222 22222211 11111111
        // 98765432 10987654 32109876 54321098 76543210
        UEDATX = keymedia_consumer_keys[0];
        UEDATX = (keymedia_consumer_keys[1] << 2) | ((keymedia_consumer_keys[0] >> 8) & 0x03);
        UEDATX = (keymedia_consumer_keys[2] << 4) | ((keymedia_consumer_keys[1] >> 6) & 0x0F);
        UEDATX = (keymedia_consumer_keys[3] << 6) | ((keymedia_consumer_keys[2] >> 4) & 0x3F);
        UEDATX = keymedia_consumer_keys[3] >> 2;
        UEDATX = keymedia_system_keys[0];
        UEDATX = keymedia_system_keys[1];
        UEDATX = keymedia_system_keys[2];
        UEINTX = 0x3A;
        SREG = intr_state;
}


int usb_rawhid_class::available(void)
{
	uint8_t n=0, intr_state;

	intr_state = SREG;
	cli();
	if (usb_configuration) {
		UENUM = RAWHID_RX_ENDPOINT;
		n = UEBCLX;
	}
	SREG = intr_state;
	return n;
}

// receive a packet, with timeout
int usb_rawhid_class::recv(void *ptr, uint16_t timeout)
{
	uint8_t *buffer = (uint8_t *)ptr;
        uint8_t intr_state;

        // if we're not online (enumerated and configured), error
        if (!usb_configuration) return -1;
        intr_state = SREG;
        cli();
        rawhid_rx_timeout_count = timeout;
        UENUM = RAWHID_RX_ENDPOINT;
        // wait for data to be available in the FIFO
        while (1) {
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                if (rawhid_rx_timeout_count == 0) return 0;
                if (!usb_configuration) return -1;
                intr_state = SREG;
                cli();
                UENUM = RAWHID_RX_ENDPOINT;
        }
        // read bytes from the FIFO
        #if (RAWHID_RX_SIZE >= 64)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 63)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 62)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 61)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 60)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 59)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 58)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 57)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 56)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 55)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 54)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 53)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 52)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 51)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 50)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 49)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 48)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 47)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 46)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 45)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 44)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 43)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 42)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 41)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 40)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 39)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 38)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 37)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 36)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 35)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 34)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 33)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 32)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 31)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 30)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 29)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 28)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 27)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 26)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 25)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 24)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 23)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 22)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 21)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 20)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 19)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 18)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 17)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 16)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 15)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 14)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 13)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 12)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 11)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 10)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 9)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 8)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 7)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 6)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 5)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 4)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 3)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 2)
        *buffer++ = UEDATX;
        #endif
        #if (RAWHID_RX_SIZE >= 1)
        *buffer++ = UEDATX;
        #endif
        // release the buffer
        UEINTX = 0x6B;
        SREG = intr_state;
        return RAWHID_RX_SIZE;
}


// send a packet, with timeout
int usb_rawhid_class::send(const void *ptr, uint16_t timeout)
{
	const uint8_t *buffer = (const uint8_t *)ptr;
        uint8_t intr_state;

        // if we're not online (enumerated and configured), error
        if (!usb_configuration) return -1;
        intr_state = SREG;
        cli();
        rawhid_tx_timeout_count = timeout;
        UENUM = RAWHID_TX_ENDPOINT;
        // wait for the FIFO to be ready to accept data
        while (1) {
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                if (rawhid_tx_timeout_count == 0) return 0;
                if (!usb_configuration) return -1;
                intr_state = SREG;
                cli();
                UENUM = RAWHID_TX_ENDPOINT;
        }
        // write bytes from the FIFO
        #if (RAWHID_TX_SIZE >= 64)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 63)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 62)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 61)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 60)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 59)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 58)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 57)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 56)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 55)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 54)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 53)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 52)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 51)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 50)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 49)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 48)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 47)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 46)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 45)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 44)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 43)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 42)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 41)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 40)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 39)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 38)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 37)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 36)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 35)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 34)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 33)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 32)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 31)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 30)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 29)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 28)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 27)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 26)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 25)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 24)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 23)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 22)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 21)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 20)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 19)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 18)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 17)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 16)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 15)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 14)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 13)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 12)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 11)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 10)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 9)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 8)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 7)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 6)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 5)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 4)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 3)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 2)
        UEDATX = *buffer++;
        #endif
        #if (RAWHID_TX_SIZE >= 1)
        UEDATX = *buffer++;
        #endif
        // transmit it now
        UEINTX = 0x3A;
        SREG = intr_state;
        return RAWHID_TX_SIZE;
}






static volatile uint8_t prev_byte=0;

void usb_serial_class::begin(long speed)
{
	// make sure USB is initialized
	usb_init();
	uint16_t begin_wait = (uint16_t)millis();
	while (1) {
		if (usb_configuration) {
			delay(200);  // a little time for host to load a driver
			return;
		}
		if (usb_suspended) {
			uint16_t begin_suspend = (uint16_t)millis();
			while (usb_suspended) {
				// must remain suspended for a while, because
				// normal USB enumeration causes brief suspend
				// states, typically under 0.1 second
				if ((uint16_t)millis() - begin_suspend > 250) {
					return;
				}
			}
		}
		// ... or a timout (powered by a USB power adaptor that
		// wiggles the data lines to keep a USB device charging)
		if ((uint16_t)millis() - begin_wait > 2500) return;
	}
	prev_byte = 0;
}

void usb_serial_class::end()
{
	usb_shutdown();
	delay(25);
}



// number of bytes available in the receive buffer
int usb_serial_class::available()
{
        uint8_t c;

	c = prev_byte;  // assume 1 byte static volatile access is atomic
	if (c) return 1;
	c = readnext();
	if (c) {
		prev_byte = c;
		return 1;
	}
	return 0;
}

// get the next character, or -1 if nothing received
int usb_serial_class::read()
{
	uint8_t c;

	c = prev_byte;
	if (c) {
		prev_byte = 0;
		return c;
	}
	c = readnext();
	if (c) return c;
	return -1;
}

int usb_serial_class::peek()
{
	uint8_t c;
	
	c = prev_byte;
	if (c) return c;
	c = readnext();
	if (c) {
		prev_byte = c;
		return c;
	}
	return -1;
}

// get the next character, or 0 if nothing
uint8_t usb_serial_class::readnext(void)
{
        uint8_t c, intr_state;

        // interrupts are disabled so these functions can be
        // used from the main program or interrupt context,
        // even both in the same program!
        intr_state = SREG;
        cli();
        if (!usb_configuration) {
                SREG = intr_state;
                return 0;
        }
        UENUM = DEBUG_RX_ENDPOINT;
try_again:
        if (!(UEINTX & (1<<RWAL))) {
                // no packet in buffer
                SREG = intr_state;
                return 0;
        }
        // take one byte out of the buffer
        c = UEDATX;
	if (c == 0) {
		// if we see a zero, discard it and
		// discard the rest of this packet
		UEINTX = 0x6B;
		goto try_again;
	}
        // if this drained the buffer, release it
        if (!(UEINTX & (1<<RWAL))) UEINTX = 0x6B;
        SREG = intr_state;
        return c;
}

// discard any buffered input
void usb_serial_class::flush()
{
        uint8_t intr_state;

        if (usb_configuration) {
                intr_state = SREG;
                cli();
		UENUM = DEBUG_RX_ENDPOINT;
                while ((UEINTX & (1<<RWAL))) {
                        UEINTX = 0x6B;
                }
                SREG = intr_state;
        }
	prev_byte = 0;
}

// transmit a character.
#if ARDUINO >= 100
size_t usb_serial_class::write(uint8_t c)
#else
void usb_serial_class::write(uint8_t c)
#endif
{
        //static uint8_t previous_timeout=0;
        uint8_t timeout, intr_state;

        // if we're not online (enumerated and configured), error
        if (!usb_configuration) goto error;
        // interrupts are disabled so these functions can be
        // used from the main program or interrupt context,
        // even both in the same program!
        intr_state = SREG;
        cli();
        UENUM = DEBUG_TX_ENDPOINT;
        // if we gave up due to timeout before, don't wait again
#if 0
	// this seems to be causig a lockup... why????
        if (previous_timeout) {
                if (!(UEINTX & (1<<RWAL))) {
                        SREG = intr_state;
                        return;
                }
                previous_timeout = 0;
        }
#endif
        // wait for the FIFO to be ready to accept data
        timeout = UDFNUML + TRANSMIT_TIMEOUT;
        while (1) {
                // are we ready to transmit?
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                // have we waited too long?  This happens if the user
                // is not running an application that is listening
                if (UDFNUML == timeout) {
                        //previous_timeout = 1;
			goto error;
		}
                // has the USB gone offline?
                if (!usb_configuration) goto error;
                // get ready to try checking again
                intr_state = SREG;
                cli();
                UENUM = DEBUG_TX_ENDPOINT;
        }
        // actually write the byte into the FIFO
        UEDATX = c;
        // if this completed a packet, transmit it now!
        if (!(UEINTX & (1<<RWAL))) {
		UEINTX = 0x3A;
        	debug_flush_timer = 0;
	} else {
        	debug_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	}
        SREG = intr_state;
#if ARDUINO >= 100
	return 1;
#endif
error:
#if ARDUINO >= 100
	setWriteError();
	return 0;
#else
	return;
#endif
}


// These are Teensy-specific extensions to the Serial object

// immediately transmit any buffered output.
// This doesn't actually transmit the data - that is impossible!
// USB devices only transmit when the host allows, so the best
// we can do is release the FIFO buffer for when the host wants it
void usb_serial_class::send_now(void)
{
        uint8_t intr_state;

        intr_state = SREG;
        cli();
        if (debug_flush_timer) {
                UENUM = DEBUG_TX_ENDPOINT;
		while ((UEINTX & (1<<RWAL))) {
			UEDATX = 0;
		}
                UEINTX = 0x3A;
                debug_flush_timer = 0;
        }
        SREG = intr_state;
}

uint32_t usb_serial_class::baud(void)
{
	return ((uint32_t)DEBUG_TX_SIZE * 10000 / DEBUG_TX_INTERVAL);
}

uint8_t usb_serial_class::stopbits(void)
{
	return 1;
}

uint8_t usb_serial_class::paritytype(void)
{
	return 0;
}

uint8_t usb_serial_class::numbits(void)
{
	return 8;
}

uint8_t usb_serial_class::dtr(void)
{
	return 1;
}

uint8_t usb_serial_class::rts(void)
{
	return 1;
}

usb_serial_class::operator bool()
{
	if (usb_configuration) return true;
	return false;
}



// Preinstantiate Objects //////////////////////////////////////////////////////

usb_serial_class	Serial = usb_serial_class();
usb_keyboard_class	Keyboard = usb_keyboard_class();
usb_rawhid_class	RawHID = usb_rawhid_class();

