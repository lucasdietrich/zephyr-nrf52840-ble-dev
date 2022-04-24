#ifndef _BLE_XIAOMI_ADV_RECORD_H_
#define _BLE_XIAOMI_ADV_RECORD_H_

#include <stdint.h>

#include <bluetooth/addr.h>

typedef struct {
	/**
	 * @brief Record device address
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Time when the measurements were retrieved
	 */
	uint32_t time;
	
	/**
	 * @brief rssi
	 */
	uint8_t rssi;
} __attribute__((packed)) xiaomi_adv_record_t;


_Static_assert(sizeof(xiaomi_adv_record_t) < 256, "xiaomi_adv_record_t is too large");

#endif /* _BLE_XIAOMI_ADV_RECORD_H_ */