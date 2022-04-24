#include <zephyr.h>

#include <string.h>

#include "ipc_uart/ipc.h"

#define MESSAGE "Hello World!"

int main(void)
{
	ipc_data_t data;

	memcpy(data.buf, MESSAGE, sizeof(MESSAGE));
	data.size = sizeof(MESSAGE);

	for (;;) {
		ipc_send_data(&data);

		k_sleep(K_SECONDS(5));
	}
}