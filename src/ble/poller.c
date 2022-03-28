#include "poller.h"

#include <stddef.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/addr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

void thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(ble_thread, 0x1000, thread, NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);

/*___________________________________________________________________________*/

/**
 * @brief What frequency the scan should be performed passivly or actively
 * 
 * 0 : Passive scan
 * 1 : Always active scan
 * n : One active scan every n scans, others are passive
 */
#define ACTIVE_SCAN_PERIODICITY 1

#define XIAOMI_MANUFACTURER_ADDR_STR "A4:C1:38:00:00:00"
// #define XIAOMI_MANUFACTURER_ADDR ((bt_addr_t) { .val = { 0x00, 0x00, 0x00, 0x38, 0xC1, 0xA4 } })

#define XIAOMI_LYWSD03MMC_NAME "LYWSD03MMC"
#define XIAOMI_LYWSD03MMC_NAME_SIZE	(sizeof(XIAOMI_LYWSD03MMC_NAME) - 1)

#define XIAOMI_MAX_DEVICES 20

typedef enum {
	STATE_NONE = 0,
	STATE_DISCOVERED,
	STATE_CONNECTED,
	STATE_DATA,
	STATE_DISCONNECTED,
} xiaomi_state_t;

typedef struct {
	bt_addr_le_t addr;
	xiaomi_state_t state;

	struct k_sem sem;
	struct bt_conn *conn;
	
	struct {
		uint32_t uptime; 
		
		int16_t temperature; /* 1e-2 °C */
		uint8_t humidity; /* 1 % */
		uint16_t battery; /* 1 mV */
		
		bool valid;
	} measurements;

	uint32_t write_count;

	uint32_t last_process;
} xiaomi_context_t;

xiaomi_context_t devices[XIAOMI_MAX_DEVICES];
uint8_t devices_count = 0U;

void activate_cb(struct bt_conn *conn,
		 uint8_t err,
		 struct bt_gatt_write_params *params);

static uint8_t measurement_cb(struct bt_conn *conn,
			      uint8_t err,
			      struct bt_gatt_read_params *params,
			      const void *data,
			      uint16_t length);

// write phandle 0x38 to activate measurements
static uint8_t data[2] = { 0x01, 0x00 }; // could be any data
static struct bt_gatt_write_params write_params = {
	.func = activate_cb,
	.handle = 0x38U,
	.offset = 0x00U,
	.data = data,
	.length = sizeof(data),
};
static struct bt_gatt_read_params read_params = {
	.func = measurement_cb,
	.handle_count = 1U,
	.single.handle = 0x36U,
};

static void show_device_measurements(xiaomi_context_t *ctx)
{
	if ((ctx != NULL) && (ctx->measurements.valid == true)) {
		LOG_INF("T : %hd.%02hd °C [ %d ], H %hhu %%, bat %u mV",
			ctx->measurements.temperature / 100,
			ctx->measurements.temperature % 100,
			ctx->measurements.temperature,
			ctx->measurements.humidity,
			ctx->measurements.battery);
	} else {
		LOG_INF("(%x) no valid measurements", (uint32_t)ctx);
	}
}

/*___________________________________________________________________________*/

xiaomi_context_t *get_context_from_conn(struct bt_conn *conn)
{
	xiaomi_context_t *dev = NULL;

	for (uint8_t i = 0; i < devices_count; i++) {
		if (devices[i].conn == conn) {
			dev = &devices[i];
		}
	}

	return dev;
}

xiaomi_context_t *find_device_context(const bt_addr_le_t *addr)
{
	xiaomi_context_t *dev = NULL;

	for (uint8_t i = 0; i < devices_count; i++) {
		if (bt_addr_le_cmp(&devices[i].addr, addr) == 0) {
			dev = &devices[i];
		}
	}

	return dev;
}

static void reset_device_context(xiaomi_context_t *ctx)
{
	ctx->state = STATE_NONE;
	ctx->conn = NULL;
	ctx->measurements.temperature = 0;
	ctx->measurements.humidity = 0U;
	ctx->measurements.battery = 0U;
	ctx->write_count = 0U;
	ctx->last_process = 0U;
	ctx->measurements.valid = false;

	k_sem_init(&ctx->sem, 1, 1);
}

static void register_device(const bt_addr_le_t *addr)
{
	xiaomi_context_t *device = find_device_context(addr);
	if (device == NULL) {
		if (devices_count >= XIAOMI_MAX_DEVICES) {
			LOG_ERR("Too many devices (%u)", devices_count);
			return;
		}

		device = &devices[devices_count];
		devices_count++;

		bt_addr_le_copy(&device->addr, addr);
	}

	reset_device_context(device);

	device->state = STATE_DISCOVERED;
}

/*___________________________________________________________________________*/

static int initialize(void)
{
	/* Initialize the Bluetooth Subsystem */
	int ret = bt_enable(NULL);
	if (ret != 0) {
		LOG_INF("Bluetooth init failed (ret %d)", ret);
		return ret;
	}

	LOG_INF("Bluetooth initialized %d", 0);

	return 0;
}

static bool bt_addr_manufacturer_match(const bt_addr_t *addr, const bt_addr_t *mf_prefix)
{
	return memcmp(&addr->val[3], &mf_prefix->val[3], 3U) == 0;
}

static bool adv_data_cb(struct bt_data *data, void *user_data)
{
	bool *is_xiaomi = user_data;

	if ((data->type == BT_DATA_NAME_COMPLETE) &&
	    (data->data_len == XIAOMI_LYWSD03MMC_NAME_SIZE) &&
	    (memcmp(data->data, XIAOMI_LYWSD03MMC_NAME, data->data_len) == 0)) {
		*is_xiaomi = true;
		return false;
	}

	return true;
}

static int read_measurements(xiaomi_context_t *ctx);

void activate_cb(struct bt_conn *conn,
		 uint8_t err,
		 struct bt_gatt_write_params *params)
{
	xiaomi_context_t *ctx = get_context_from_conn(conn);

	ctx->write_count++;

	LOG_DBG("conn %x, err %hhu", (uint32_t) conn, err);

	if (err == 0) {
		read_measurements(ctx);
	}
}

int activate_measurements(xiaomi_context_t *ctx)
{
	int ret;

	ret = bt_gatt_write(ctx->conn, &write_params);
	if (ret) {
		LOG_ERR("Failed to write (ret %d)", ret);

		ret = bt_conn_disconnect(ctx->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (ret != 0) {
			LOG_ERR("Failed to disconnect (ret %d)", ret);
		}
	}

	return ret;
}

static void interpret_measurements(xiaomi_context_t *ctx, const uint8_t *data)
{
	const uint16_t uTemperature = data[0] | (data[1] << 8);

	ctx->measurements.temperature = *((int16_t *)&uTemperature);
	ctx->measurements.humidity = data[2];
	ctx->measurements.battery = data[3] | (data[4] << 8);
}

static uint8_t measurement_cb(struct bt_conn *conn,
			      uint8_t err,
			      struct bt_gatt_read_params *params,
			      const void *data,
			      uint16_t length)
{
	bool disconnect = false;
	const uint8_t empty_data[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	xiaomi_context_t *const ctx = get_context_from_conn(conn);

	LOG_DBG("conn = %x, len = %u", (uint32_t)conn, length);
	LOG_HEXDUMP_DBG(data, length, "measurement");

	if ((length == 5U) && (memcmp(data, empty_data, length) != 0)) {
		interpret_measurements(ctx, data);
		disconnect = true;
	} else if ((length == 3U) && (ctx->write_count != 5)) {
		LOG_WRN("Invalid measurement data length %u", length);
		activate_measurements(ctx);
	} else if (length == 0) {
		disconnect = read_measurements(ctx) != 0;
	} else {
		LOG_WRN("Write count exceeded = %u", ctx->write_count);
		disconnect = true;
	}

	if (disconnect == true) {
		/* close the connection */
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	return BT_GATT_ITER_STOP;
}

static int read_measurements(xiaomi_context_t *ctx)
{
	ctx->state = STATE_DATA;

	int ret = bt_gatt_read(ctx->conn, &read_params);
	if (ret != 0) {
		LOG_ERR("Failed to read measurements (ret %d)", ret);
	}

	return ret;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	static char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	bool disconnect = true;

	xiaomi_context_t *ctx = get_context_from_conn(conn);
	if (err != 0) {
		LOG_ERR("(%x) Failed to connect to %s (%u)",
			(uint32_t)conn, log_strdup(addr), err);

		bt_conn_unref(conn);
		conn = NULL;

		if (ctx != NULL) {
			ctx->conn = NULL;
			k_sem_give(&ctx->sem);
		}
	} else {
		if (ctx != NULL) {
			LOG_INF("(%x) Connected: %s",
				(uint32_t)conn, log_strdup(addr));
			ctx->state = STATE_CONNECTED;

			disconnect = read_measurements(ctx) != 0;
		}

		if (disconnect == true) {
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	static char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("(%x) Disconnected: %s (reason 0x%02x)", (uint32_t) conn, 
		log_strdup(addr), reason);

	bt_conn_unref(conn);

	xiaomi_context_t *ctx = get_context_from_conn(conn);
	if (ctx != NULL) {
		ctx->state = STATE_DISCONNECTED;

		ctx->conn = NULL;
		
		k_sem_give(&ctx->sem);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr,
			 int8_t rssi,
			 uint8_t type,
			 struct net_buf_simple *ad)
{
	static char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	// Parse advertisement data
	bool is_xiaomi = false;

	bt_addr_t mf;
	bt_addr_from_str(XIAOMI_MANUFACTURER_ADDR_STR, &mf);

	if (bt_addr_manufacturer_match(&addr->a, &mf) == true) {
		/* We're only interested in connectable events */
		// if (type == BT_HCI_ADV_IND || type == BT_HCI_ADV_DIRECT_IND) { ... }

		bt_data_parse(ad, adv_data_cb, &is_xiaomi);
	}

	if (is_xiaomi == true) {
		register_device(addr);
	}
}

static int scan_xiaomi_devices(uint8_t scan_type, k_timeout_t timeout)
{
	int ret;

	if ((scan_type != BT_LE_SCAN_TYPE_PASSIVE) && (scan_type != BT_LE_SCAN_TYPE_ACTIVE)) {
		LOG_ERR("Invalid scan type %u", scan_type);
		return -EINVAL;
	}

	struct bt_le_scan_param scan_param = {
		.type = scan_type,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	ret = bt_le_scan_start(&scan_param, device_found);
	if (ret) {
		LOG_ERR("Starting scanning failed (ret %d)", ret);
		return ret;
	}

	k_sleep(timeout);

	ret = bt_le_scan_stop();
	if (ret != 0) {
		LOG_ERR("Stopping scanning failed (ret %d)", ret);
	}

	return ret;
}

static const struct bt_conn_le_create_param conn_create_param = {
	.options = BT_CONN_LE_OPT_NONE,
	.interval = BT_GAP_SCAN_FAST_INTERVAL,
	.window = BT_GAP_SCAN_FAST_INTERVAL,
	.interval_coded = 0,
	.window_coded = 0,
	.timeout = 0,
};

static const struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(
	BT_GAP_INIT_CONN_INT_MIN, /* Minimum Connection Interval (N * 1.25 ms) */
	BT_GAP_INIT_CONN_INT_MAX, /* Maximum Connection Interval (N * 1.25 ms) */
	0, /* Connection Latency */
	1000); /* Supervision Timeout (N * 10 ms), default 400 */

static int retrieve_xiaomi_measurements(xiaomi_context_t *ctx)
{
	int ret;
	
	LOG_DBG("ctx = %x", (uint32_t) ctx);

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = k_sem_take(&ctx->sem, K_NO_WAIT);
	if (ret != 0) {
		/* operation already pending */
		LOG_ERR("(%x) Operation already pending", (uint32_t) ctx);
		return ret;
	}

	ret = bt_conn_le_create(&ctx->addr,
				BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT,
				&ctx->conn);
	if (ret < 0) {
		LOG_ERR("(%x) Failed to create connection (ret %d)", 
			(uint32_t)ctx, ret);
		return ret;
	}

	return 0;
}

static const char *get_scan_type_str(uint8_t scan_type) {
	switch (scan_type) {
	case BT_LE_SCAN_TYPE_PASSIVE:
		return "BT_LE_SCAN_TYPE_PASSIVE";
	case BT_LE_SCAN_TYPE_ACTIVE:
		return "BT_LE_SCAN_TYPE_ACTIVE";
	default:
		return "<unknown scan type>";
	}
}

static uint8_t get_scan_type(void)
{
	uint8_t scan_type = BT_LE_SCAN_TYPE_PASSIVE;

#if ACTIVE_SCAN_PERIODICITY != 0
	static uint32_t remaining = ACTIVE_SCAN_PERIODICITY;

	if (remaining <= 1U) {
		scan_type = BT_LE_SCAN_TYPE_ACTIVE;
		remaining = ACTIVE_SCAN_PERIODICITY;
	} else {
		remaining--;
	}
#endif /* ACTIVE_SCAN_PERIODICITY == 0 */

	LOG_DBG("scan_type = %s", log_strdup(get_scan_type_str(scan_type)));

	return scan_type;
}

static struct k_poll_event events[XIAOMI_MAX_DEVICES];

void thread(void *_a, void *_b, void *_c)
{
	initialize();

	for (;;) {
		const uint8_t iteration_scan_type = get_scan_type();
		scan_xiaomi_devices(iteration_scan_type, K_SECONDS(20));

		// list found devices
		for (uint32_t i = 0; i < devices_count; i++) {
			xiaomi_context_t *dev = &devices[i];

			static char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(&dev->addr, addr_str, sizeof(addr_str));

			LOG_INF("Device %u: %s", i, log_strdup(addr_str));
		}

		uint32_t devices_remaining = devices_count;

		/* start measurements for all devices */
		for (uint32_t i = 0; i < devices_count; i++) {
			xiaomi_context_t *device = &devices[i];

			retrieve_xiaomi_measurements(device);

			k_poll_event_init(&events[i], K_POLL_TYPE_SEM_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY, &device->sem);
		}

		while (devices_remaining > 0) {
			LOG_DBG("Polling for %u remaining devices over %u",
				devices_remaining, devices_count);
			int ret = k_poll(events, devices_remaining, K_FOREVER);
			// iterate over all events and take semaphores (start from the end)
			for (int i = devices_remaining - 1; i >= 0; i--) {
				if (events[i].state == K_POLL_STATE_SEM_AVAILABLE) {
					xiaomi_context_t *device = &devices[i];

					ret = k_sem_take(&device->sem, K_NO_WAIT);
					if (ret == 0) {
						devices_remaining--;
					}

					// shift next events to the left
					for (int j = i; j < devices_remaining; j++) {
						events[j] = events[j + 1];
					}
				}
			}
		}

		/* we have all the semaphores, we iterate over all devices and
		 * retrieve measurements */
		for (uint32_t i = 0; i < devices_count; i++) {
			show_device_measurements(&devices[i]);
		}
	}
}
