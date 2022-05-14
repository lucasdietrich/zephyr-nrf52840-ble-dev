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
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE	DT_ALIAS(pwm_led0)
#define PWM_LED1_NODE	DT_ALIAS(pwm_led1)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM0_CTLR	DT_PWMS_CTLR(PWM_LED0_NODE)
#define PWM0_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM0_FLAGS	DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_CTLR	DT_INVALID_NODE
#define PWM_CHANNEL	0
#define PWM_FLAGS	0
#endif

#if DT_NODE_HAS_STATUS(PWM_LED1_NODE, okay)
#define PWM1_CTLR	DT_PWMS_CTLR(PWM_LED1_NODE)
#define PWM1_CHANNEL	DT_PWMS_CHANNEL(PWM_LED1_NODE)
#define PWM1_FLAGS	DT_PWMS_FLAGS(PWM_LED1_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_CTLR	DT_INVALID_NODE
#define PWM_CHANNEL	0
#define PWM_FLAGS	0
#endif

#define MIN_PERIOD_USEC	(USEC_PER_SEC / 64U)
#define MAX_PERIOD_USEC	USEC_PER_SEC

void main(void)
{
	const struct device *pwm;
	uint32_t max_period;
	uint32_t period;
	uint8_t dir = 0U;
	int ret;

	printk("PWM-based blinky\n");

	printk("0: ctrl: %p chan: %u flags: %u\n", DEVICE_DT_GET(PWM0_CTLR), PWM0_CHANNEL, PWM0_FLAGS);
	printk("1: ctrl: %p chan: %u flags: %u\n", DEVICE_DT_GET(PWM1_CTLR), PWM1_CHANNEL, PWM1_FLAGS);

	if (DEVICE_DT_GET(PWM0_CTLR) == DEVICE_DT_GET(PWM1_CTLR)) {
		printk("pwm_led0 and pwm_led1 use the same PWM controller\n");
	}

	pwm = DEVICE_DT_GET(PWM0_CTLR);
	if (!device_is_ready(pwm)) {
		printk("Error: PWM device %s is not ready\n", pwm->name);
		return;
	}

	/*
	 * In case the default MAX_PERIOD_USEC value cannot be set for
	 * some PWM hardware, decrease its value until it can.
	 *
	 * Keep its value at least MIN_PERIOD_USEC * 4 to make sure
	 * the sample changes frequency at least once.
	 */
	printk("Calibrating for channel %d...\n", PWM1_CHANNEL);
	max_period = MAX_PERIOD_USEC;
	while (pwm_pin_set_usec(pwm, PWM1_CHANNEL,
				max_period, max_period / 2U, PWM1_FLAGS)) {
		max_period /= 2U;
		if (max_period < (4U * MIN_PERIOD_USEC)) {
			printk("Error: PWM device "
			       "does not support a period at least %u\n",
			       4U * MIN_PERIOD_USEC);
			return;
		}
	}

	printk("Done calibrating; maximum/minimum periods %u/%u usec\n",
	       max_period, MIN_PERIOD_USEC);

	period = max_period;
	while (1) {
		pwm_pin_set_usec(pwm, PWM0_CHANNEL, 0U, 0U, PWM0_FLAGS);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		pwm_pin_set_usec(pwm, PWM1_CHANNEL, 0U, 0U, PWM1_FLAGS);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		ret = pwm_pin_set_usec(pwm, PWM0_CHANNEL,
				       period, period / 2U, PWM0_FLAGS);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		ret = pwm_pin_set_usec(pwm, PWM1_CHANNEL,
				       period, period / 2U, PWM1_FLAGS);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		period = dir ? (period * 2U) : (period / 2U);
		if (period > max_period) {
			period = max_period / 2U;
			dir = 0U;
		} else if (period < MIN_PERIOD_USEC) {
			period = MIN_PERIOD_USEC * 2U;
			dir = 1U;
		}

		k_sleep(K_SECONDS(4U));
	}
}
