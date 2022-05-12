/**
 * @file poller.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief BLE module polling for devices
 * @version 0.1
 * @date 2022-03-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _BLE_COLLECTOR_H_
#define _BLE_COLLECTOR_H_

#include <zephyr.h>

#include <bluetooth/bluetooth.h>

#include "xiaomi_record.h"

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

typedef enum {
	XIAOMI_FILTER_NONE = 0,
	XIAOMI_FILTER_RSSI,
	XIAOMI_FILTER_NAME,
	XIAOMI_FILTER_MEAS,
	XIAOMI_FILTER_MEAS_OR_RSSI,
	XIAOMI_FILTER_ANY,
	XIAOMI_FILTER_ALL,
} xiaomi_filter_t;

size_t iterate_devices(xiaomi_filter_t filter,
		       void (*callback)(xiaomi_context_t *dev,
					void *user_data),
		       void *user_data);

#endif /* _BLE_COLLECTOR_H_ */