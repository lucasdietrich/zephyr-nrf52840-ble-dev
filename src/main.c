/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 */

#include <zephyr.h>
#include "io/io.h"

void main(void)
{
	io_init();

	uint32_t i = 0;
	uint32_t j = 45;
	uint32_t k = 88;

	for (;;) {
		io_led_sig(IO_LED_BLE, 100U, (4*i) % 100);
		io_led_sig(IO_LED_IPC_RX, 60U, (4*j) % 100);
		io_led_sig(IO_LED_IPC_TX, 150U, (4*k) % 100);
		i++;
		j++;
		k++;
		k_sleep(K_MSEC(200));
	}	
}
