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
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

bt_addr_le_t devices[10];
uint32_t devices_count = 0U;

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params);

/* Discover procedures can be initiated with the use of bt_gatt_discover() API which takes the bt_gatt_discover_params struct which describes the type of discovery. The parameters also serves as a filter when setting the uuid field only attributes which matches will be discovered, in contrast setting it to NULL allows all attributes to be discovered. */
static struct bt_gatt_discover_params discover_params_all = {
	.uuid = NULL,
	.func = discover_func,
	.start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE,
	.end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
	.type = BT_GATT_DISCOVER_ATTRIBUTE
};

static struct bt_gatt_discover_params discover_params_primary = {
	.uuid = NULL,
	.func = discover_func,
	.start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE,
	.end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
	.type = BT_GATT_DISCOVER_PRIMARY
};

// static const struct bt_gatt_discover_params discover_params_primary = {
// 	.uuid = NULL,
// 	.func = discover_func,
// 	.start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE,
// 	.end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
// 	.type = BT_GATT_DISCOVER_PRIMARY
// };

static struct bt_gatt_subscribe_params subscribe_params;

// LYWSD03MMC
const char target_str[] = "A4:C1:38:A7:30:C4"; // xiaomi
// const char target_str[] = "FA:76:2E:BD:9C:BF"; // nrf9160dk_nrf52840

static struct bt_conn *default_conn = NULL;

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

void process_target(const bt_addr_le_t *addr)
{
	int ret = bt_conn_le_create(addr,
				    BT_CONN_LE_CREATE_CONN,
				    BT_LE_CONN_PARAM_DEFAULT,
				    &default_conn);
	LOG_DBG("bt_conn_le_create = %d", ret);
}

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %p length %u\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	// show uuid
	char uuid_str[BT_UUID_STR_LEN];
	bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));

	printk("[ATTRIBUTE] handle 0x%x : uuid %s perm = %hhu\n",
	       attr->handle,
	       uuid_str,
	       attr->perm);

	return BT_GATT_ITER_CONTINUE;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (%u)", log_strdup(addr), err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		return;
	}

	if (conn == default_conn) {
		LOG_INF("Connected: %s\n", log_strdup(addr));

		err = bt_gatt_discover(default_conn, &discover_params_all);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}

	// bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)\n", log_strdup(addr), reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	if (exists(addr) == false) {
		append(addr);

		if (rssi > -55) {
			printk("[ CLOSE ]");
		}

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

	k_sleep(K_SECONDS(5));

	err = bt_le_scan_stop();
	printk("bt_le_scan_stop = %d\n", err);

	// show number of devices found
	printk("Found %d devices\n", devices_count);

	// convert target_str to address and connect to it if it is in the devices list
	bt_addr_le_t target_addr;
	bt_addr_le_from_str(target_str, "public", &target_addr);
	process_target(&target_addr);

	for(;;) {
		k_sleep(K_SECONDS(1));
	}
}