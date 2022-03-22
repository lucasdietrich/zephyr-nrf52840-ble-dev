#include <zephyr.h>

#include <stddef.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

// const char target_str[] = "A4:C1:38:68:05:63"; // xiaomi (unconfigured)
const char target_str[] = "A4:C1:38:A7:30:C4"; // xiaomi (configured)

static struct bt_conn *default_conn = NULL;

K_SEM_DEFINE(sem_conn, 0, 1);
K_SEM_DEFINE(sem_data, 0, 1);
K_SEM_DEFINE(sem_disconn, 0, 1);

void connect_target(const bt_addr_le_t *addr)
{
	int ret = bt_conn_le_create(addr,
				    BT_CONN_LE_CREATE_CONN,
				    BT_LE_CONN_PARAM_DEFAULT,
				    &default_conn);
	LOG_DBG("bt_conn_le_create = %d", ret);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (%u)", log_strdup(addr), err);

		bt_conn_unref(conn);
		conn = NULL;

		return;
	}

	if (conn == default_conn) {
		LOG_INF("Connected: %s", log_strdup(addr));

		k_sem_give(&sem_conn);
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

	bt_conn_unref(conn);
	conn = NULL;

	k_sem_give(&sem_disconn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void show_data(const uint8_t *data, uint16_t len)
{
	uint16_t uTemperature = data[0] | (data[1] << 8);
	int16_t Temperature = *((int16_t*) &uTemperature); // 1e-2 °C
	uint8_t Hygrometry = data[2]; // %
	uint16_t batteryVoltage = data[3] | (data[4] << 8); // mV
	
	LOG_INF("T : %hd.%02hd °C [ %u ], H %hhu %%, bat %u mV",
		Temperature / 100, Temperature % 100,
		(uint32_t)uTemperature, Hygrometry, 
		(uint32_t) batteryVoltage);
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

		k_sem_give(&sem_data);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		LOG_DBG("[UNSUBSCRIBED] = %d", 0);
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	LOG_HEXDUMP_DBG(data, length, "notify");

	if (length == 5U) {
		show_data(data, length);

		k_sem_give(&sem_data);
	}

	return BT_GATT_ITER_CONTINUE;
}

int main(void)
{
	/* Initialize the Bluetooth Subsystem */
	int ret = bt_enable(NULL);
	if (ret) {
		LOG_INF("Bluetooth init failed (ret %d)", ret);
		return 0;
	}

	LOG_INF("Bluetooth initialized %d", 0);

	bt_addr_le_t target_addr;
	bt_addr_le_from_str(target_str, "public", &target_addr);
	connect_target(&target_addr);

	k_sem_take(&sem_conn, K_FOREVER);

	// struct bt_gatt_subscribe_params subscribe_params = {
	// 	.notify = notify_func,
	// 	.value = BT_GATT_CCC_NOTIFY, // BT_GATT_CCC_NOTIFY
	// 	.ccc_handle = 0x36,
	// };
	// ret = bt_gatt_subscribe(default_conn, &subscribe_params);
	// if (ret && ret != -EALREADY) {
	// 	LOG_ERR("Subscribe failed (err %d)", ret);
	// } else {
	// 	LOG_INF("[SUBSCRIBED] = %d", 0);
	// }

	struct bt_gatt_read_params read_params = {
		.func = handle_cb_0x36,
		.handle_count = 1U,
		.single.handle = 0x36U,
	};
	ret = bt_gatt_read(default_conn, &read_params);
	LOG_DBG("bt_gatt_read = %d", ret);

	k_sem_take(&sem_data, K_SECONDS(20));

	// k_sleep(K_SECONDS(20));

	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	k_sem_take(&sem_disconn, K_FOREVER);

	for (;;) {
		k_sleep(K_SECONDS(1));
	}
}