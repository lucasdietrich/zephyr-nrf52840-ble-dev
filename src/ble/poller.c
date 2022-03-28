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
LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

void thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(ble_thread, 0x1000, thread, NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);

#define XIAOMI_MANUFACTURER_ADDR_STR "A4:C1:38:00:00:00"
// #define XIAOMI_MANUFACTURER_ADDR ((bt_addr_t) { .val = { 0x00, 0x00, 0x00, 0x38, 0xC1, 0xA4 } })

#define XIAOMI_LYWSD03MMC_NAME "LYWSD03MMC"
#define XIAOMI_LYWSD03MMC_NAME_SIZE	(sizeof(XIAOMI_LYWSD03MMC_NAME) - 1)

#define XIAOMI_MAX_DEVICES 20

struct bt_conn *gconn;

typedef enum {
	STATE_NONE = 0,
	STATE_DISCOVERED,
	STATE_CONNECTED,
	STATE_DATA,
	STATE_DISCONNECTED,
} xiaomi_state_t;

#define STATE_ACTIVE STATE_DISCOVERED

typedef struct {
	bt_addr_le_t addr;
	xiaomi_state_t state;

	struct k_sem sem;
	struct bt_conn *conn;
	
	struct {
		int16_t temperature; /* 1e-2 °C */
		uint8_t humidity; /* 1 % */
		uint16_t battery; /* 1 mV */
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
	LOG_INF("T : %hd.%02hd °C [ %d ], H %hhu %%, bat %u mV",
		ctx->measurements.temperature / 100, ctx->measurements.temperature % 100,
		ctx->measurements.temperature, ctx->measurements.humidity,
		ctx->measurements.battery);
}

xiaomi_context_t *get_context_from_conn(struct bt_conn *conn)
{
	for (uint8_t i = 0; i < devices_count; i++) {
		if (devices[i].conn == conn) {
			return &devices[i];
		}
	}

	LOG_WRN("conn not found: %x", (uint32_t)conn);

	return NULL;
}

static void register_device(const bt_addr_le_t *addr)
{
	if (devices_count >= XIAOMI_MAX_DEVICES) {
		LOG_ERR("Too many devices (%u)", devices_count);
		return;
	}

	xiaomi_context_t *device = &devices[devices_count];

	bt_addr_le_copy(&device->addr, addr);
	device->state = STATE_DISCOVERED;
	k_sem_init(&device->sem, 1, 1);

	devices_count++;
}

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
	LOG_DBG("conn = %x, len = %u", (uint32_t)conn, length);

	const uint8_t empty_data[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	LOG_HEXDUMP_DBG(data, length, "measurement");

	xiaomi_context_t *ctx = get_context_from_conn(conn);

	if ((length == 5U) && (memcmp(data, empty_data, length) != 0)) {
		interpret_measurements(ctx, data);

		/* close the connection */
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	} else if (ctx->write_count != 5) {
		LOG_WRN("Invalid measurement data length %u", length);

		// activate_measurements(ctx);

		read_measurements(ctx);
	} else {
		LOG_WRN("Write count exceeded = %u", ctx->write_count);

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

	xiaomi_context_t *ctx = get_context_from_conn(conn);
	if (err) {
		LOG_ERR("(%x) Failed to connect to %s (%u)", (uint32_t) conn, log_strdup(addr), err);

		bt_conn_unref(conn);
		conn = NULL;

		if (ctx != NULL) {
			ctx->conn = NULL;
			k_sem_give(&ctx->sem);
		}
		return;
	}

	if (ctx == NULL) {
		goto cancel;
	}

	LOG_INF("(%x) Connected: %s", (uint32_t) conn, log_strdup(addr));

	ctx->state = STATE_CONNECTED;

	if (read_measurements(ctx) != 0) {
		goto cancel;
	}

	return;

cancel:
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
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

static int scan(k_timeout_t timeout)
{
	int ret;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	ret = bt_le_scan_start(&scan_param, device_found);
	if (ret) {
		LOG_INF("Starting scanning failed (ret %d)", ret);
		return ret;
	}
	
	k_sleep(timeout);

	ret = bt_le_scan_stop();
	if (ret != 0) {
		LOG_ERR("Stopping scanning failed (ret %d)", ret);
	}

	return ret;
}


const struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(
	BT_GAP_INIT_CONN_INT_MIN, /* Minimum Connection Interval (N * 1.25 ms) */
	BT_GAP_INIT_CONN_INT_MAX, /* Maximum Connection Interval (N * 1.25 ms) */
	0, /* Connection Latency */
	1000); /* Supervision Timeout (N * 10 ms), default 400 */

static int retrieve_xiaomi_measurements(xiaomi_context_t *ctx)
{
	LOG_DBG("ctx = %x", (uint32_t) ctx);

	if (ctx == NULL) {
		return -EINVAL;
	}

	int ret;

	ret = k_sem_take(&ctx->sem, K_NO_WAIT);
	if (ret != 0) {
		/* operation already pending */
		LOG_ERR("Operation already pending, ctx %x", (uint32_t) ctx);
		return ret;
	}

	ret = bt_conn_le_create(&ctx->addr,
				BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT,
				&ctx->conn);
	if (ret < 0) {
		LOG_ERR("Failed to create connection (ret %d)", ret);
		return ret;
	}

	return 0;
}

// const char target_str[] = "A4:C1:38:A7:30:C4"; // xiaomi (normal)
const char target_str[] = "A4:C1:38:68:05:63"; // xiaomi (normal)

void thread(void *_a, void *_b, void *_c)
{
	int ret;

	initialize();

	scan(K_SECONDS(20));

	bt_addr_le_t target_addr;
	bt_addr_le_from_str(target_str, "public", &target_addr);
	xiaomi_context_t *device = NULL;

	// list found devices
	for (uint32_t i = 0; i < devices_count; i++) {
		xiaomi_context_t *dev = &devices[i];

		static char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(&dev->addr, addr_str, sizeof(addr_str));

		LOG_INF("Device %u: %s", i, log_strdup(addr_str));

		if (bt_addr_cmp((const bt_addr_t *)&dev->addr.a,
				(const bt_addr_t *)&target_addr.a) == 0) {
			device = dev;
		}
	}

	for (;;) {
		for (uint32_t i = 0; i < devices_count; i++) {
			ret = retrieve_xiaomi_measurements(&devices[i]);
			if (ret != 0) {
				LOG_ERR("Failed to retrieve measurements (ret %d)", ret);
			}

			k_sem_take(&devices[i].sem, K_FOREVER);

			show_device_measurements(&devices[i]);

			k_sem_give(&devices[i].sem);
		}

		LOG_INF("==========================================================");

		k_sleep(K_SECONDS(1));
	}



	// struct k_poll_event events[5];

	// scan and poll continuously, first scan is active, next scan are active
	for (;;) {
		// ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		k_sleep(K_SECONDS(1));
	}
}
