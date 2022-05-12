#include "utils.h"

#include "crc32_ieee.h"

uint32_t get_uptime_sec(void)
{
	int64_t now = k_uptime_get();

	return (uint32_t)(now / MSEC_PER_SEC);
}

uint32_t crc_calculate32(uint32_t *buf, size_t len)
{
	return crc32_ieee_u32(buf, len);
}