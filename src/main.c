#include <kernel.h>

#include <stddef.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	uint32_t i = 0;
	
	LOG_INF("Hello, world! %u", i);
	return 0;
}