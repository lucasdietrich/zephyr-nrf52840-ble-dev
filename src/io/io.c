#include "io.h"

#include <device.h>
#include <devicetree.h>
#include <drivers/counter.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(io, LOG_LEVEL_DBG);

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(user_button), gpios);
static struct gpio_callback button_cb_data;

static void (*button_callback)(io_button_t button,
			       void *user_data);
static void *button_callback_user_data;

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb,
			   uint32_t pins)
{
	if (button_callback != NULL) {
		LOG_DBG("Button pressed, cb: %p (user data: %p)",
			button_callback, button_callback_user_data);
		button_callback(IO_BUTTON_USER, button_callback_user_data);
	} else {
		LOG_WRN("Button pressed but no callback registered, cb: %p",
			button_callback);
	}
}

static int button_init(void)
{
        int ret = -EIO;

        /* set up button */
        if (!device_is_ready(button.port)) {
                LOG_ERR("Error: button device %s is not ready",
                        button.port->name);
                goto exit;
        }

        ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
        if (ret != 0) {
                LOG_ERR("Error %d: failed to configure %s pin %d",
                        ret, button.port->name, button.pin);
                goto exit;
        }

        ret = gpio_pin_interrupt_configure_dt(&button,
                                              GPIO_INT_EDGE_TO_ACTIVE);
        if (ret != 0) {
                LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
                        ret, button.port->name, button.pin);
                goto exit;
        }

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_DBG("Set up button at %s pin %d", button.port->name, button.pin);

exit:
	return ret;
}

#define PWM_LED_BLE_NODE	DT_ALIAS(ble_led)
#define PWM_LED_IPCRX_NODE	DT_ALIAS(ipc_rx_led)
#define PWM_LED_IPCTX_NODE	DT_ALIAS(ipc_tx_led)

#if DT_NODE_HAS_STATUS(PWM_LED_BLE_NODE, okay)
#define PWM_LED_BLE_CTLR	DT_PWMS_CTLR(PWM_LED_BLE_NODE)
#define PWM_LED_BLE_CHANNEL	DT_PWMS_CHANNEL(PWM_LED_BLE_NODE)
#define PWM_LED_BLE_FLAGS	DT_PWMS_FLAGS(PWM_LED_BLE_NODE)
#else
#error "Unsupported board: ble-led devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(PWM_LED_IPCRX_NODE, okay)
#define PWM_LED_IPCRX_CTLR	DT_PWMS_CTLR(PWM_LED_IPCRX_NODE)
#define PWM_LED_IPCRX_CHANNEL	DT_PWMS_CHANNEL(PWM_LED_IPCRX_NODE)
#define PWM_LED_IPCRX_FLAGS	DT_PWMS_FLAGS(PWM_LED_IPCRX_NODE)
#else
#error "Unsupported board: ipc-rx-led devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(PWM_LED_IPCTX_NODE, okay)
#define PWM_LED_IPCTX_CTLR	DT_PWMS_CTLR(PWM_LED_IPCTX_NODE)
#define PWM_LED_IPCTX_CHANNEL	DT_PWMS_CHANNEL(PWM_LED_IPCTX_NODE)
#define PWM_LED_IPCTX_FLAGS	DT_PWMS_FLAGS(PWM_LED_IPCTX_NODE)
#else
#error "Unsupported board: ipc-tx-led devicetree alias is not defined"
#endif

/* all leds should use the same PWM controller */
#define PWM_CTRL PWM_LED_BLE_CTLR

static const struct device *pwm = DEVICE_DT_GET(PWM_CTRL);

#define PWM_PERIOD_US (100U)

// #define COUNTER_NODE DT_NODELABEL(timer1)
#define COUNTER_NODE DT_ALIAS(counter)

static const struct device *counter = DEVICE_DT_GET(COUNTER_NODE);

#define ALARM_CFG_INITIALIZE(cb, user) \
{ \
	.callback = cb, \
	.ticks = 0U, \
	.user_data = user, \
	.flags = 0U, \
}

static void led_alarm_cb(const struct device *dev,
			 uint8_t chan_id, uint32_t ticks,
			 void *user_data);

static struct counter_alarm_cfg alarms_cfg[3U] = {
	ALARM_CFG_INITIALIZE(led_alarm_cb, &alarms_cfg[0U]),
	ALARM_CFG_INITIALIZE(led_alarm_cb, &alarms_cfg[1U]),
	ALARM_CFG_INITIALIZE(led_alarm_cb, &alarms_cfg[2U]),
};

static int get_led_chan_infos(io_led_t led, pwm_flags_t *flags, uint32_t *channel)
{
	int ret = 0;

	__ASSERT(flags != NULL, "Invalid flags");
	__ASSERT(channel != NULL, "Invalid channel");

	switch (led) {
	case IO_LED_BLE:
		*channel = PWM_LED_BLE_CHANNEL;
		*flags = PWM_LED_BLE_FLAGS;
		break;
	case IO_LED_IPC_RX:
		*channel = PWM_LED_IPCRX_CHANNEL;
		*flags = PWM_LED_IPCRX_FLAGS;
		break;
	case IO_LED_IPC_TX:
		*channel = PWM_LED_IPCTX_CHANNEL;
		*flags = PWM_LED_IPCTX_FLAGS;
		break;
	default:
		ret = -EINVAL;
		goto exit;
	}
exit:
	return ret;
}

static void led_alarm_cb(const struct device *dev,
			 uint8_t chan_id, uint32_t ticks,
			 void *user_data)
{
	struct counter_alarm_cfg *acfg = user_data;

	io_led_t led = (io_led_t)(acfg - alarms_cfg);

	pwm_flags_t flags;
	uint32_t channel;

	if (get_led_chan_infos(led, &flags, &channel) == 0U) {
		pwm_pin_set_usec(pwm, channel, PWM_PERIOD_US, 0U, flags);
	}
}

static int leds_init(void)
{
	int ret = -EIO;

	if (device_is_ready(pwm) == false) {
		LOG_ERR("PWM device %s (%p) is not ready", log_strdup(pwm->name), pwm);
		goto exit;
	}

	const struct device *pwm_ipcrx = DEVICE_DT_GET(PWM_LED_IPCRX_CTLR);
	const struct device *pwm_ipctx = DEVICE_DT_GET(PWM_LED_IPCTX_CTLR);
	if ((pwm != pwm_ipcrx) || (pwm != pwm_ipctx)) {
		LOG_ERR("Different PWM controllers for leds !! ble: %p ipc-rx: %p ipc-tx: %p",
			pwm, pwm_ipcrx, pwm_ipctx);
		goto exit;
	}

	if (device_is_ready(counter) == false) {
		LOG_ERR("Counter device %s (%p) is not ready",
			log_strdup(counter->name), counter);
		goto exit;
	}

	/* enable timer counter and set default state for all leds */
	uint8_t channels_count = counter_get_num_of_channels(counter);
	if (channels_count < ARRAY_SIZE(alarms_cfg)) {
		LOG_ERR("Counter device %s (%p) has not enough channels (%hhu)",
			log_strdup(counter->name), counter, channels_count);
		/* consider switching to a different RTC */
	}

	counter_start(counter);

	io_led_sig(IO_LED_BLE, SYS_FOREVER_MS, 0U);
	io_led_sig(IO_LED_IPC_RX, SYS_FOREVER_MS, 0U);
	io_led_sig(IO_LED_IPC_TX, SYS_FOREVER_MS, 0U);
exit:
	return ret;
}

/**
 * @brief Convert bright value to PWM duty cycle
 * 
 * @param brightness value in range [0, 100] (forced to 100 if greater)
 * @return uint32_t pulse width in microseconds
 */
static uint32_t brightness_to_pulse_us(uint8_t brightness)
{
	if (brightness > 100U) {
		brightness = 100U;
		LOG_WRN("Brightness %u is too high, limiting to 100", brightness);
	}

	return (brightness * PWM_PERIOD_US) / 100U;
}

int io_led_sig(io_led_t led, uint32_t duration_ms, uint8_t brightness)
{
	int ret;

	pwm_flags_t flags;
	uint32_t channel;
	uint32_t pulse = 0U;

	ret = get_led_chan_infos(led, &flags, &channel);
	if (ret != 0) {
		ret = -EINVAL;
		goto exit;
	}

	struct counter_alarm_cfg *const alarm_cfg = &alarms_cfg[led];
	counter_cancel_channel_alarm(counter, (uint8_t)led);

	if (duration_ms != 0U) {
		pulse = brightness_to_pulse_us(brightness);
	}

	ret = pwm_pin_set_usec(pwm, channel, PWM_PERIOD_US, pulse, flags);
	if (ret) {
		LOG_ERR("pwm_pin_set_usec failed: %d", ret);
		goto exit;
	}

	if (duration_ms != SYS_FOREVER_MS) {
		alarm_cfg->ticks = counter_us_to_ticks(counter, duration_ms * 1000U);
		ret = counter_set_channel_alarm(counter, (uint8_t)led, alarm_cfg);
		if (ret) {
			LOG_ERR("counter_set_channel_alarm failed: %d", ret);
			goto exit;
		}
	}

exit:
	return ret;
}

void io_init(void)
{
        button_init();
	leds_init();
}

void io_button_attach(void (*callback)(io_button_t button,
				       void *user_data),
		      void *user_data)
{
	button_callback_user_data = user_data;
	button_callback = callback;
}