#include "collector.h"

#include <stddef.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/addr.h>
#include <bluetooth/gap.h>

#include "../uart_ipc/ipc_frame.h"
#include "../uart_ipc/ipc.h"

#include "xiaomi_record.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

void thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(ble_thread, 0x1000, thread, NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);

/*___________________________________________________________________________*/

#define XIAOMI_MANUFACTURER_ADDR_STR "A4:C1:38:00:00:00"
// #define XIAOMI_MANUFACTURER_ADDR ((bt_addr_t) { .val = { 0x00, 0x00, 0x00, 0x38, 0xC1, 0xA4 } })

#define XIAOMI_CUSTOMATC_NAME_STARTS_WITH "ATC_"
#define XIAOMI_CUSTOMATC_NAME_STARTS_WITH_SIZE	\
	(sizeof(XIAOMI_CUSTOMATC_NAME_STARTS_WITH) - 1)

#define XIAOMI_CUSTOMATC_ADV_PAYLOAD_SIZE sizeof(struct xiaomi_atc_custom_adv_payload)

/* https://github.com/pvvx/ATC_MiThermometer#custom-format-all-data-little-endian */
struct xiaomi_atc_custom_adv_payload
{
	uint16_t    UUID;   // = 0x181A, GATT Service 0x181A Environmental Sensing
	uint8_t     MAC[6]; // [0] - lo, .. [6] - hi digits
	int16_t     temperature;    // x 0.01 degree
	uint16_t    humidity;       // x 0.01 %
	uint16_t    battery_mv;     // mV
	uint8_t     battery_level;  // 0..100 %
	uint8_t     counter;        // measurement count
	uint8_t     flags;  // GPIO_TRG pin (marking "reset" on circuit board) flags: 
			    // bit0: Reed Switch, input
			    // bit1: GPIO_TRG pin output value (pull Up/Down)
			    // bit2: Output GPIO_TRG pin is controlled according to the set parameters
			    // bit3: Temperature trigger event
			    // bit4: Humidity trigger event
} __attribute__((packed));

struct xiaomi_atc_custom_adv {
	uint8_t     size;   // = 19
	uint8_t     uid;    // = 0x16, 16-bit UUID

	struct xiaomi_atc_custom_adv_payload payload;
};

typedef struct {
	/**
	 * @brief BLE Device address 
	 */
	bt_addr_le_t addr;

	/**
	 * @brief name;
	 */
	char name[CONFIG_BT_DEVICE_NAME_MAX];

	/**
	 * @brief Number of measurements successfully retrieved
	 */
	uint32_t measurements_count;

	/**
	 * @brief Last retrieved record
	 */
	xiaomi_measurements_t measurements;

	/**
	 * @brief Time of the last attempt of measurements retrieval
	 */
	uint32_t last_measurement;

	/**
	 * @brief flags
	 *
	 */
	struct {
		uint8_t rssi : 1;
		uint8_t name : 1;
		uint8_t meas : 1;
	} ready;
} xiaomi_context_t;

/**
 * @brief Table of all xiaomi devices, this table is filled by the scan process
 */
xiaomi_context_t devices[CONFIG_XIAOMI_MAX_DEVICES];

/**
 * @brief Nmber of devices in the table "devices"
 */
uint8_t devices_count = 0U;

K_MUTEX_DEFINE(devices_mutex);

/**
 * @brief Semaphore to trigger data
 */
K_SEM_DEFINE(ipc_sem, 0U, 1U);

static const char *scan_type_to_str(uint8_t scan_type) {
	switch (scan_type) {
	case BT_LE_SCAN_TYPE_PASSIVE:
		return "BT_LE_SCAN_TYPE_PASSIVE";
	case BT_LE_SCAN_TYPE_ACTIVE:
		return "BT_LE_SCAN_TYPE_ACTIVE";
	default:
		return "<unknown scan type>";
	}
}

static inline uint32_t get_uptime_sec(void)
{
	int64_t now = k_uptime_get();

	return (uint32_t)(now / MSEC_PER_SEC);
}

/*___________________________________________________________________________*/


static void reset_device_context(xiaomi_context_t *ctx)
{
	memset(ctx, 0x00U, sizeof(xiaomi_context_t));
}


static xiaomi_context_t *get_device_context(const bt_addr_le_t *addr)
{
	xiaomi_context_t *dev = NULL;

	for (uint8_t i = 0; i < devices_count; i++) {
		if (bt_addr_le_cmp(&devices[i].addr, addr) == 0) {
			dev = &devices[i];
			break;
		}
	}

	return dev;
}

static xiaomi_context_t *get_or_register_device(const bt_addr_le_t *addr)
{
	xiaomi_context_t *device = get_device_context(addr);

	/* device not already registered, we add it */
	if (device == NULL) {
		if (devices_count >= CONFIG_XIAOMI_MAX_DEVICES) {
			LOG_ERR("Too many devices (%u)", devices_count);
			static xiaomi_context_t tmp_dev; /* data lost */
			device = &tmp_dev;
		}

		device = &devices[devices_count];
		devices_count++;

		reset_device_context(device);

		bt_addr_le_copy(&device->addr, addr);
	}

	return device;
}

typedef enum {
	XIAOMI_FILTER_NONE = 0,
	XIAOMI_FILTER_RSSI,
	XIAOMI_FILTER_NAME,
	XIAOMI_FILTER_MEAS,
	XIAOMI_FILTER_ANY,
	XIAOMI_FILTER_ALL,
} xiaomi_filter_t;

static bool device_matches_filter(xiaomi_context_t *dev, xiaomi_filter_t filter)
{
	bool matches = false;

	switch (filter) {
	case XIAOMI_FILTER_RSSI:
		matches = dev->ready.rssi == 1U;
		break;
	case XIAOMI_FILTER_NAME:
		matches = dev->ready.name == 1U;
		break;
	case XIAOMI_FILTER_MEAS:
		matches = dev->ready.meas == 1U;
		break;
	case XIAOMI_FILTER_ALL:
		matches = dev->ready.rssi == 1U &&
			dev->ready.name == 1U &&
			dev->ready.meas == 1U;
	case XIAOMI_FILTER_ANY:
		matches = dev->ready.rssi == 1U ||
			dev->ready.name == 1U ||
			dev->ready.meas == 1U;
	default:
		break;
	}

	return matches;
}

static size_t iterate_devices(xiaomi_filter_t filter,
			      void (*callback)(xiaomi_context_t *dev,
					       void *user_data),
			      void *user_data)
{
	size_t count = 0U;
	xiaomi_context_t *dev;

	for (dev = devices; dev < devices + ARRAY_SIZE(devices); dev++) {
		if (device_matches_filter(dev, filter) == true) {
			callback(dev, user_data);
		}
	}

	return count;
}

/*___________________________________________________________________________*/

static bool bt_addr_manufacturer_match(const bt_addr_t *addr,
				       const bt_addr_t *mf_prefix)
{
	return memcmp(&addr->val[3], &mf_prefix->val[3], 3U) == 0;
}

static bool adv_data_cb(struct bt_data *data, void *user_data)
{
	xiaomi_context_t *dev = (xiaomi_context_t *)user_data;

	switch (data->type) {
	case BT_DATA_NAME_COMPLETE:
	{
		if ((data->data_len >= XIAOMI_CUSTOMATC_NAME_STARTS_WITH_SIZE) &&
		    (memcmp(data->data,
			    XIAOMI_CUSTOMATC_NAME_STARTS_WITH,
			    XIAOMI_CUSTOMATC_NAME_STARTS_WITH_SIZE) == 0)) {

			/* copy device name */
			size_t copy_len = MIN(data->data_len, sizeof(dev->name) - 1);
			memcpy(dev->name, data->data, copy_len);
			dev->name[copy_len] = '\0';
			dev->ready.name = 1U;

			LOG_DBG("name: %s", log_strdup(dev->name));
		}
	}
	break;
	case BT_DATA_SVC_DATA16:
	{
		if (data->data_len == XIAOMI_CUSTOMATC_ADV_PAYLOAD_SIZE) {
			struct xiaomi_atc_custom_adv_payload *const payload = 
				(struct xiaomi_atc_custom_adv_payload *) data->data;

			if (payload->UUID == BT_UUID_ESS_VAL) {
				/* parsing payload */
				// LOG_HEXDUMP_DBG(data->data, data->data_len,
				// 		"enviroment sensor adv data");
				dev->measurements.battery_level = payload->battery_level;
				dev->measurements.battery_mv = payload->battery_mv;
				dev->measurements.humidity = payload->humidity;
				dev->measurements.temperature = payload->temperature;
				dev->last_measurement = get_uptime_sec();
				dev->ready.meas = 1U;
			}
		}
	}
	break;
	default:
		break;
	}

	return dev->ready.meas && dev->ready.name;
}

static void device_found(const bt_addr_le_t *addr,
			 int8_t rssi,
			 uint8_t type,
			 struct net_buf_simple *ad)
{
	bt_addr_t mf;
	bt_addr_from_str(XIAOMI_MANUFACTURER_ADDR_STR, &mf);

	if (bt_addr_manufacturer_match(&addr->a, &mf) == true) {

		/* TODO filter by type */
		// char mac_str[BT_ADDR_STR_LEN];
		// bt_addr_le_to_str(addr, mac_str, sizeof(mac_str));
		// LOG_DBG("mac: %s", log_strdup(mac_str));
		// LOG_HEXDUMP_DBG(ad->data, ad->len, "advertisement");

		k_mutex_lock(&devices_mutex, K_FOREVER);

		xiaomi_context_t *dev = get_or_register_device(addr);
		dev->measurements.rssi = rssi;
		dev->ready.rssi = 1U;
		bt_data_parse(ad, adv_data_cb, dev);

		k_mutex_unlock(&devices_mutex);
		k_sem_give(&ipc_sem);
	}
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

/**
 * @brief Get the type for the next scan.
 * 
 * Note: First scan is always active.
 * 
 * @return uint8_t 
 */
static uint8_t get_scan_type(void)
{
	static uint8_t scan_type = BT_LE_SCAN_TYPE_PASSIVE;

	switch (scan_type) {
	case BT_LE_SCAN_TYPE_PASSIVE:
		scan_type = BT_LE_SCAN_TYPE_ACTIVE;
		break;
	case BT_LE_SCAN_TYPE_ACTIVE:
		scan_type = BT_LE_SCAN_TYPE_PASSIVE;
		break;
	default:
		break;
	}

	return scan_type;
}

/**
 * @brief Get the scan duration for the given scan_type (in seconds)
 * 
 * @param scan_type 
 * @return uint32_t 
 */
static uint32_t get_scan_duration(uint8_t scan_type)
{
	uint32_t duration = 0U;

	switch (scan_type) {
	case BT_LE_SCAN_TYPE_PASSIVE:
		duration = CONFIG_PASSIVE_SCAN_DURATION;
		break;
	case BT_LE_SCAN_TYPE_ACTIVE:
		duration = CONFIG_ACTIVE_SCAN_DURATION;
		break;
	default:
		break;
	}
	return duration;
}

static int scan_start(uint8_t scan_type)
{
	int ret;

	if ((scan_type != BT_LE_SCAN_TYPE_PASSIVE) && 
	    (scan_type != BT_LE_SCAN_TYPE_ACTIVE)) {
		LOG_ERR("Invalid scan type %u", scan_type);
		return -EINVAL;
	}

	struct bt_le_scan_param scan_param = {
		.type = scan_type,
		.options = BT_LE_SCAN_OPT_NONE, /* don't filter duplicates */
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	ret = bt_le_scan_start(&scan_param, device_found);
	if (ret) {
		LOG_ERR("Starting scanning failed (ret %d)", ret);
		return ret;
	}

	return ret;
}

static int scan_stop(void)
{
	int ret = bt_le_scan_stop();
	if (ret != 0) {
		LOG_ERR("Stopping scanning failed (ret %d)", ret);
	}
	return ret;
}

static void scan_mode_update(void)
{
	static bool scanning = false;
	static uint32_t last_scan_start = 0;
	static uint32_t scan_duration = 0U;

	uint32_t now = get_uptime_sec();

	if (scanning == true) {
		uint32_t diff = now - last_scan_start;
		if (diff > scan_duration) {
			LOG_DBG("Scanning stopped, lasted %u seconds", diff);
			scan_stop();
			scanning = false;
		}
	}

	if (scanning == false) {
		const uint8_t scan_type = get_scan_type();
		scan_duration = get_scan_duration(scan_type);
		scanning = true;
		last_scan_start = now;

		scan_start(scan_type);

		LOG_DBG("%s started for %u seconds", 
			log_strdup(scan_type_to_str(scan_type)), scan_duration);
	}
}

static void df_build_dev_iterate_cb(xiaomi_context_t *dev, void *user_data)
{
	xiaomi_dataframe_t *const df = (xiaomi_dataframe_t *)user_data;
	xiaomi_record_t *const rec = &df->records[df->count];

	/* copy addr into record */
	bt_addr_le_copy(&rec->addr, &dev->addr);
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(&rec->addr, addr_str, sizeof(addr_str));

	/* copy measurement time */
	rec->time = dev->last_measurement;

	if (dev->ready.meas == 1U) {
		memcpy(&rec->measurements,
		       &dev->measurements,
		       sizeof(xiaomi_measurements_t));
	}

	LOG_DBG("%s: %s: rssi: %d temp: %hd hum: %hu bat: %hu %%: %hhu",
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

	k_mutex_lock(&devices_mutex, K_FOREVER);

	/* initialize frame */
	ret = ipc_allocate_frame(&frame);
	if (ret != 0) {
		LOG_ERR("Failed to allocate frame (ret %d)", ret);
		goto exit;
	}

	xiaomi_dataframe_t *dataframe = (xiaomi_dataframe_t *)frame->data.buf;
	clear_data_frame(dataframe);

	/* build dataframe */
	iterate_devices(XIAOMI_FILTER_ANY, df_build_dev_iterate_cb, dataframe);

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
	k_mutex_unlock(&devices_mutex);

	return ret;
}

static bool is_frame_ready(void)
{
	static uint32_t ipc_last_timestamp = 0;

	bool ready = false;
	const uint32_t now = get_uptime_sec();

	if (k_sem_take(&ipc_sem, K_SECONDS(1)) == 0) {
		ready = (now - ipc_last_timestamp) > CONFIG_DATAFRAME_MIN_INTERVAL;
	}

	if (ready) {
		ipc_last_timestamp = now;
	}

	return ready;
}

void thread(void *_a, void *_b, void *_c)
{
	initialize();

	for (;;) {
		scan_mode_update();

		if (is_frame_ready() == true) {
			send_data_frame();
		} else {
			k_sleep(K_SECONDS(1));
		}
	}
}
