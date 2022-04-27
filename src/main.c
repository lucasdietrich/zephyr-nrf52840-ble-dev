#include <zephyr.h>

#include <string.h>

#include "uart_ipc/ipc.h"


K_MSGQ_DEFINE(msgq, sizeof(ipc_frame_t), 4U, 4U);

int main(void)
{
	ipc_frame_t frame;

	ipc_attach_rx_msgq(&msgq);

	for (;;) {
		if (k_msgq_get(&msgq, &frame, K_FOREVER) == 0) {
			ipc_send_data(&frame.data);
		}
	}
}