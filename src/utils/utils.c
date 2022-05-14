#include "utils.h"

#include "crc32_ieee.h"

#include "math.h"

uint32_t get_uptime_sec(void)
{
	int64_t now = k_uptime_get();

	return (uint32_t)(now / MSEC_PER_SEC);
}

uint32_t crc_calculate32(uint32_t *buf, size_t len)
{
	return crc32_ieee_u32(buf, len);
}

uint8_t rssi_to_brightness(int8_t rssi)
{
	const int8_t min_rssi = -88; // 93
	const int8_t max_rssi = -19;
	
	/* contraint rssi */
	rssi = MAX(MIN(rssi, max_rssi), min_rssi);

	/* normalize rssi to 0-99 */
	const uint8_t nrssi = (uint8_t)((rssi - min_rssi) * 99U / (max_rssi - min_rssi));

	/* +1 to be sure that the brightness is always > 0 */
	return nrssi + 1U;
}