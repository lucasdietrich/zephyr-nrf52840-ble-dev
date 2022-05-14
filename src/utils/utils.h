#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <zephyr.h>

uint32_t get_uptime_sec(void);

/**
 * @brief Convert RSSI to brightness value for a LED
 * 
 * There is a nonlinear relationship between the RSSI and the actual
 *   distance between the sensor. We decided to use a linear
 *   relationship to calculate the brightness.
 * 
 * @param rssi 
 * @return uint8_t 
 */
uint8_t rssi_to_brightness(int8_t rssi);

#endif /* _UTILS_H_ */