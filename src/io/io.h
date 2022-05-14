#ifndef _IO_H_
#define _IO_H_

#include <zephyr.h>

typedef enum {
	IO_LED_BLE,
	IO_LED_IPC_RX,
	IO_LED_IPC_TX,
} io_led_t;

typedef enum {
	IO_BUTTON_USER,
} io_button_t;

/**
 * @brief Initialize buttons, counter and LEDs
 */
void io_init(void);

/**
 * @brief Set the callback on button press
 * 
 * @param callback 
 * @param user_data 
 */
void io_button_attach(void (*callback)(io_button_t button,
				       void *user_data),
		      void *user_data);

/**
 * @brief Generate a pulse on the specified LED
 * 
 * @param led LED to generate pulse on : IO_LED_BLE, IO_LED_IPC_RX or IO_LED_IPC_TX
 * @param duration_ms duration of the pulse in milliseconds
 * 	- 0 : led off
 * 	- SYS_FOREVER_MS : led on forever
 * @param brightness brightness of the pulse in range [0, 100] (forced to 100 if greater)
 * @return int 
 */
int io_led_sig(io_led_t led, uint32_t duration_ms, uint8_t brightness);

#endif  /* _IO_H_ */