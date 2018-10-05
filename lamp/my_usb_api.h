#ifndef MyUSBAPI_h_
#define MyUSBAPI_h_

#include <inttypes.h>

#include "keylayouts.h"
#include "Print.h"
#include "Stream.h"

class usb_keyboard_class : public Print
{
	public:
	void begin(void) { }
	void end(void) { }
	virtual size_t write(uint8_t);
	using Print::write;
	void write_unicode(uint16_t unicode) { write_keycode(unicode_to_keycode(unicode)); }
	void set_modifier(uint16_t);
	void set_key1(uint8_t);
	void set_key2(uint8_t);
	void set_key3(uint8_t);
	void set_key4(uint8_t);
	void set_key5(uint8_t);
	void set_key6(uint8_t);
	void set_media(uint16_t c) {
                if (c == 0) {
                        keymedia_release_all();
                } else if (c >= 0xE400 && c <= 0xE7FF) {
                        press(c);
                }
	}
	void send_now(void);
	void press(uint16_t n);
	void release(uint16_t n);
	void releaseAll(void);
	private:
	KEYCODE_TYPE unicode_to_keycode(uint16_t unicode);
	KEYCODE_TYPE deadkey_to_keycode(KEYCODE_TYPE keycode);
	uint8_t keycode_to_modifier(KEYCODE_TYPE keycode);
	uint8_t keycode_to_key(KEYCODE_TYPE keycode);
	void presskey(uint8_t key, uint8_t modifier);
	void releasekey(uint8_t key, uint8_t modifier);
	void write_keycode(KEYCODE_TYPE key);
	void write_key(KEYCODE_TYPE code);
	void press_consumer_key(uint16_t key);
	void release_consumer_key(uint16_t key);
	void press_system_key(uint8_t key);
	void release_system_key(uint8_t key);
	void keymedia_release_all(void);
	void keymedia_send(void);
	uint8_t utf8_state;
	uint16_t unicode_wchar;
};

extern usb_keyboard_class Keyboard;

#endif
