#include "crc32_ieee.h"

uint32_t crc_calculate32(uint32_t *buf, size_t len)
{
	return crc32_ieee_u32(buf, len);
}