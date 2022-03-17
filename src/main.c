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

#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

bt_addr_le_t devices[10];
uint32_t devices_count = 0U;

K_SEM_DEFINE(discovery_sem, 0, 1);

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

// static struct bt_gatt_discover_params discover_params_primary = {
// 	.uuid = NULL,
// 	.func = discover_func,
// 	.start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE,
// 	.end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
// 	.type = BT_GATT_DISCOVER_PRIMARY
// };

// static struct bt_gatt_subscribe_params subscribe_params;

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

// static uint8_t notify_func(struct bt_conn *conn,
// 			   struct bt_gatt_subscribe_params *params,
// 			   const void *data, uint16_t length)
// {
// 	if (!data) {
// 		printk("[UNSUBSCRIBED]\n");
// 		params->value_handle = 0U;
// 		return BT_GATT_ITER_STOP;
// 	}

// 	printk("[NOTIFICATION] data %p length %u\n", data, length);
// 	LOG_HEXDUMP_DBG(data, length, "data");

// 	return BT_GATT_ITER_CONTINUE;
// }

static void show_data(const uint8_t *data, uint16_t len)
{
	LOG_HEXDUMP_DBG(data, len, "data");

	uint16_t uTemperature = data[0] | (data[1] << 8);
	int16_t Temperature = *((int16_t*) &uTemperature); // 1e-2 °C
	uint8_t Hygrometry = data[2]; // %
	uint16_t batteryVoltage = data[3] | (data[4] << 8); // mV
	
	LOG_INF("T : %hd.%02hd °C [ %u ], H %hhu %%, bat %u mV",
		Temperature / 100, Temperature % 100,
		(uint32_t)uTemperature, Hygrometry, 
		(uint32_t) batteryVoltage);

	// static char float_str[20];
	// float fTemperature = Temperature / 100.0; // °C
	// sprintf(float_str, "%.2f °C", fTemperature);
	// LOG_INF("Temperature %s", log_strdup(float_str));
}

uint8_t handle_cb_0x36(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_read_params *params,
		       const void *data, uint16_t length)
{
	LOG_DBG("conn %x err %hhx params %x data %x length %u",
		(uint32_t)conn, err, (uint32_t)params,
		(uint32_t)data, (uint32_t)length);
	LOG_HEXDUMP_INF(data, length, "data");

	if (length == 5U) {
		show_data(data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (!attr) {
		LOG_INF("Discover complete %d", 0);
		(void)memset(params, 0, sizeof(*params));

		// read phandle 0x36
		struct bt_gatt_read_params read_params = {
			.func = handle_cb_0x36,
			.handle_count = 1U,
			.single.handle = 0x36U,
		};
		int ret = bt_gatt_read(default_conn, &read_params);
		LOG_DBG("bt_gatt_read = %d", ret);

		return BT_GATT_ITER_STOP;
	}

	// show uuid
	static char uuid_str[BT_UUID_STR_LEN];
	bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));

	LOG_DBG("[ATTRIBUTE] handle 0x%x : uuid %s perm = %hhu",
	       attr->handle,
	       log_strdup(uuid_str),
	       attr->perm);
	
	// subscribe to handle 0x36
	// if ((bt_gatt_attr_value_handle(attr) == 0x36U) || (attr->handle == 0x36U)) {
	// 	printk("[SUBSCRIBING to handle 0x36]\n");

	// 	subscribe_params.value_handle = 0x36;
	// 	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	// 	subscribe_params.notify = notify_func;
	// 	int err = bt_gatt_subscribe(conn, &subscribe_params);
	// 	if (err && err != -EALREADY) {
	// 		printk("Subscribe failed (err %d)\n", err);
	// 	} else {
	// 		printk("[SUBSCRIBED]\n");
	// 	}
	// 	return BT_GATT_ITER_STOP;

	// }

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
		LOG_INF("Connected: %s", log_strdup(addr));

		err = bt_gatt_discover(default_conn, &discover_params_all);
		if (err) {
			LOG_ERR("Discover failed(err %d)", err);
			return;
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	static char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", log_strdup(addr), reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&discovery_sem);
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
			LOG_INF("[ CLOSE ] %d", 0);
		}

		static char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

		LOG_INF("Device found: %s (RSSI %d) ad len = %d", log_strdup(addr_str), rssi, ad->len);
		LOG_HEXDUMP_DBG(ad->data, ad->len, "ad");
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

	LOG_INF("Starting Observer %d", 0);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized %d", 0);

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		LOG_INF("Starting scanning failed (err %d)", err);
		return;
	}

	LOG_INF("Scanning... %d", 0);

	k_sleep(K_SECONDS(5));

	err = bt_le_scan_stop();
	LOG_INF("bt_le_scan_stop = %d", err);

	// show number of devices found
	LOG_INF("Found %d devices", devices_count);

	// convert target_str to address and connect to it if it is in the devices list
	bt_addr_le_t target_addr;
	bt_addr_le_from_str(target_str, "public", &target_addr);
	process_target(&target_addr);

	LOG_INF("Main wait ... %d", 0);
	

	for(;;) {
		k_sleep(K_SECONDS(1));
	}
}