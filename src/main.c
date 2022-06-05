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

#include "uart_ipc/ipc_frame.h"
#include "uart_ipc/ipc.h"

#include "ble/collector.h"
#include "ble/xiaomi_record.h"

#include "utils/utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/**
 * @brief Semaphore to trigger data
 */
K_SEM_DEFINE(ipc_sem, 0U, 1U);

static void df_build_dev_iterate_cb(xiaomi_context_t *dev, void *user_data)
{
	xiaomi_dataframe_t *const df = (xiaomi_dataframe_t *)user_data;
	xiaomi_record_t *const rec = &df->records[df->count];

	/* copy addr into record */
	bt_addr_le_copy(&rec->addr, &dev->addr);
	char addr_str[BT_ADDR_STR_LEN];
	bt_addr_to_str(&rec->addr.a, addr_str, sizeof(addr_str));

	/* copy measurement time */
	rec->time = dev->last_measurement;

	if (dev->ready.meas == 1U || dev->ready.rssi) {
		memcpy(&rec->measurements,
		       &dev->measurements,
		       sizeof(xiaomi_measurements_t));
	}

	LOG_DBG("%s (%s) rssi: %d temp: %hd hum: %hu bat: %hu mv %%: %hhu",
		log_strdup(addr_str), log_strdup(dev->name),
		(int)rec->measurements.rssi,
		rec->measurements.temperature, rec->measurements.humidity,
		rec->measurements.battery_mv, rec->measurements.battery_level);

	/* clear measurement validity */
	dev->ready.rssi = 0U;
	dev->ready.name = 0U;
	dev->ready.meas = 0U;

	df->count++;
}

static int clear_data_frame(xiaomi_dataframe_t *dataframe)
{
	if (dataframe == NULL) {
		return -EINVAL;
	}

	memset(dataframe, 0, sizeof(xiaomi_dataframe_t));

	return 0;
}

static int send_data_frame(void)
{
	int ret;
	ipc_frame_t *frame;

	/* initialize frame */
	ret = ipc_allocate_frame(&frame);
	if (ret != 0) {
		LOG_ERR("Failed to allocate frame (ret %d)", ret);
		goto exit;
	}

	xiaomi_dataframe_t *dataframe = (xiaomi_dataframe_t *)frame->data.buf;
	clear_data_frame(dataframe);

	/* build dataframe */
	iterate_devices(XIAOMI_FILTER_MEAS_OR_RSSI, df_build_dev_iterate_cb, dataframe);

	/* finalize frame */
	dataframe->time = get_uptime_sec();
	frame->data.size = sizeof(xiaomi_dataframe_t);

	LOG_DBG("count = %u, frame_time = %u",
		dataframe->count, dataframe->time);

	/* send frame */
	ret = ipc_send_frame(frame);
	if (ret != 0) {
		LOG_ERR("Failed to send frame (ret %d)", ret);
		goto exit;
	}

	ret = 0U;

exit:
	return ret;
}

void ipc_event_cb(ipc_event_t ev, void *user_data)
{
	switch(ev) {
		case IPC_LL_FRAME_RECEIVED:
			io_led_sig(IO_LED_IPC_RX, 100U, 100U);
			break;
		case IPC_LL_FRAME_SENT:
			io_led_sig(IO_LED_IPC_TX, 100U, 100U);
			break;
		default:
			break;
	};
}

int main(void)
{
	io_init();
	
	uint32_t last_time = (uint32_t)-1;

	ipc_register_event_callback(ipc_event_cb, NULL);

	for (;;) {
		if (k_sem_take(&ipc_sem, K_FOREVER) == 0) {
			const uint32_t now = get_uptime_sec();

			if (now - last_time > CONFIG_DATAFRAME_MIN_INTERVAL) {
				last_time = now;
				send_data_frame();
			}
		}
	}
}
