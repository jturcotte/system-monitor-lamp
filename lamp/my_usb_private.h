#ifndef my_usb_private_h__
#define my_usb_private_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

/**************************************************************************
 *
 *  Configurable Options
 *
 **************************************************************************/

#define VENDOR_ID               0x16C0
#define PRODUCT_ID              0x0486
#define RAWHID_USAGE_PAGE	0xFFAB  // recommended: 0xFF00 to 0xFFFF
#define RAWHID_USAGE		0x0200  // recommended: 0x0100 to 0xFFFF

// These determine how much USB bandwidth is allocated (1=fastest)
#define RAWHID_TX_INTERVAL	1
#define RAWHID_RX_INTERVAL	1
#define DEBUG_TX_INTERVAL	1
#define DEBUG_RX_INTERVAL	2

#define TRANSMIT_FLUSH_TIMEOUT  4   /* in milliseconds */
#define TRANSMIT_TIMEOUT        25   /* in milliseconds */


/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/

#ifndef STR_PRODUCT
#define STR_PRODUCT             L"Teensyduino Keyboard/RawHID"
#endif
#ifndef STR_RAWHID
#define STR_RAWHID		L"Teensyduino RawHID"
#endif
#ifndef STR_DEBUG
#define STR_DEBUG		L"Emulated Arduino Serial"
#endif
#define ENDPOINT0_SIZE          64

// Some operating systems, especially Windows, may cache USB device
// info.  Changes to the device name may not update on the same
// computer unless the vendor or product ID numbers change, or the
// "bcdDevice" revision code is increased.


#define RAWHID_INTERFACE	0
#define RAWHID_TX_ENDPOINT	3
#define RAWHID_TX_BUFFER	EP_DOUBLE_BUFFER
#define RAWHID_TX_SIZE		64
#define RAWHID_RX_ENDPOINT	4
#define RAWHID_RX_BUFFER	EP_DOUBLE_BUFFER
#define RAWHID_RX_SIZE		64

#define DEBUG_INTERFACE		1
#define DEBUG_TX_ENDPOINT	1
#define DEBUG_TX_SIZE		64
#define DEBUG_TX_BUFFER		EP_DOUBLE_BUFFER
#define DEBUG_RX_ENDPOINT	2
#define DEBUG_RX_SIZE		32
#define DEBUG_RX_BUFFER		EP_DOUBLE_BUFFER

#define KEYBOARD_INTERFACE	2
#define KEYBOARD_ENDPOINT	5
#define KEYBOARD_SIZE		8
#define KEYBOARD_BUFFER		EP_DOUBLE_BUFFER
#define KEYBOARD_INTERVAL	1

#define KEYMEDIA_INTERFACE      3
#define KEYMEDIA_ENDPOINT       6
#define KEYMEDIA_SIZE           8
#define KEYMEDIA_BUFFER         EP_DOUBLE_BUFFER
#define KEYMEDIA_INTERVAL       4

#define NUM_ENDPOINTS		7
#define NUM_INTERFACE		4


// setup
void usb_init(void);			// initialize everything
void usb_shutdown(void);		// shut off USB

// variables
extern volatile uint8_t usb_configuration;
extern volatile uint8_t usb_suspended;
extern volatile uint8_t debug_flush_timer;
extern uint8_t keyboard_report_data[];
extern uint8_t keyboard_idle_count;
extern volatile uint8_t keyboard_leds;
extern volatile uint16_t rawhid_rx_timeout_count;
extern volatile uint16_t rawhid_tx_timeout_count;
extern uint8_t keymedia_report_data[8];
extern uint16_t keymedia_consumer_keys[4];
extern uint8_t keymedia_system_keys[3];

#ifdef __cplusplus
} // extern "C"
#endif

#endif
