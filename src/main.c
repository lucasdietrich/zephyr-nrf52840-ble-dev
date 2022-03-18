#include <kernel.h>

#include <device.h>
#include <devicetree.h>

#include <drivers/uart.h>

#include <stddef.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipc, LOG_LEVEL_DBG);

// 270
#define IPC_UART_BUFFER_SIZE ('z' - 'a' + 1)
#define IPC_UART_TX_TIMEOUT_MS (100)
#define IPC_UART_RX_TIMEOUT_MS (100)

#define IPC_UART_NODE DT_ALIAS(ipc_uart)

static const struct device *ipc_uart_dev = DEVICE_DT_GET(IPC_UART_NODE);

uint8_t double_buffer[2][IPC_UART_BUFFER_SIZE];
uint8_t *next_buffer = double_buffer[1];

struct ipc_message {
	size_t len;
	uint8_t data[IPC_UART_BUFFER_SIZE];
};

K_MSGQ_DEFINE(ipc_msgq, sizeof(struct ipc_message), 16U, 4U);

void ipc_uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int ret;
	struct ipc_message msg;

	LOG_DBG("evt->type = %u", evt->type);

	switch(evt->type) {
		case UART_TX_DONE:
			break;
		case UART_TX_ABORTED:
			break;
		case UART_RX_RDY:
			// LOG_DBG("UART_RX_RDY : buf %x off %u len %u", 
			// 	(uint32_t)evt->data.rx.buf, evt->data.rx.offset, evt->data.rx.len);
			memcpy(&msg.data, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
			msg.len = evt->data.rx.len;
			ret = k_msgq_put(&ipc_msgq, &msg, K_NO_WAIT);
			if (ret != 0) {
				LOG_ERR("k_msgq_put failed = %d", ret);
			}
			break;
		case UART_RX_BUF_REQUEST:
			uart_rx_buf_rsp(dev, next_buffer, IPC_UART_BUFFER_SIZE);
			break;
		case UART_RX_BUF_RELEASED:
			next_buffer = evt->data.rx_buf.buf;
			break;
		case UART_RX_DISABLED:
			LOG_WRN("RX disabled %d", 0);
			break;
		case UART_RX_STOPPED:
			LOG_WRN("RX stopped %d", evt->data.rx_stop.reason);
			break;
	}
}


int ipc_initialize(void)
{
	if (device_is_ready(ipc_uart_dev) == false) {
		LOG_ERR("IPC UART device not ready = %d", 0);
		return -1;
	}

	uart_callback_set(ipc_uart_dev, ipc_uart_cb, NULL);

	uart_rx_enable(ipc_uart_dev,
		       double_buffer[0],
		       IPC_UART_BUFFER_SIZE,
		       IPC_UART_RX_TIMEOUT_MS);

	return 0;
}

static uint8_t tx_buffer[100];

int main(void)
{
	ipc_initialize();

	struct ipc_message msg;

	uart_tx(ipc_uart_dev, tx_buffer, sizeof(tx_buffer), SYS_FOREVER_MS);

	for (;;) {
		k_msgq_get(&ipc_msgq, &msg, K_FOREVER);

		LOG_HEXDUMP_DBG(msg.data, msg.len, "IPC message received");
	}

	return 0;
}