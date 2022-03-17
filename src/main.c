/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

bt_addr_le_t devices[10];
uint32_t devices_count = 0U;

bool exists(const bt_addr_le_t *new_addr)
{
	for (bt_addr_le_t *addr = devices; addr < devices + devices_count; addr++) {
		if (bt_addr_le_cmp(addr, new_addr) == 0) {
			return true;
		}
	}
	return false;
}

bool append(const bt_addr_le_t *new_addr)
{
	if (devices_count == ARRAY_SIZE(devices)) {
		return false;
	}

	bt_addr_le_copy(devices + devices_count, new_addr);
	devices_count++;

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	if (exists(addr) == false) {
		append(addr);

		char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Device found: %s (RSSI %d) ad len = %d \n\t", addr_str, rssi, ad->len);
		for (size_t i = 0; i < ad->len; i++) {
			printk("%02x ", ad->data[i]);
		}
		printk("\n");
	}
}

void main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE, /* BT_LE_SCAN_OPT_FILTER_DUPLICATE */
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	printk("Starting Observer\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_scan_start(&scan_param, device_found);
	printk("\n");
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}

	printk("Scanning...\n");

	k_sleep(K_SECONDS(30));

	err = bt_le_scan_stop();
	printk("bt_le_scan_stop = %d\n", err);

	// show number of devices found
	printk("Found %d devices\n", devices_count);
}