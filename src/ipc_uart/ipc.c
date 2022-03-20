#include "ipc.h"

#include <zephyr.h>
#include <kernel.h>

#include <sys/crc.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>

#include "../utils/crc32_ieee.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ipc, LOG_LEVEL_INF);

// config
#define IPC_UART_TX_TIMEOUT_MS 50U
#define CONFIG_IPC_MEMSLAB_COUNT 2

// drivers
#define IPC_UART_NODE DT_ALIAS(ipc_uart)

static const struct device *uart_dev = DEVICE_DT_GET(IPC_UART_NODE);

// internal
#define IPC_MEMSLAB_COUNT (CONFIG_IPC_MEMSLAB_COUNT)
K_MEM_SLAB_DEFINE(tx_frame_slab, IPC_FRAME_SIZE, IPC_MEMSLAB_COUNT, 4U);

static inline int alloc_frame(ipc_frame_t **frame)
{
	return k_mem_slab_alloc(&tx_frame_slab, (void**) frame, K_NO_WAIT);
}

static inline void free_frame(ipc_frame_t **frame)
{
	k_mem_slab_free(&tx_frame_slab, (void **)frame);
}

/*___________________________________________________________________________*/

static K_SEM_DEFINE(tx_finish_sem, 1, 1);

// convert uart_event to string
static const char *uart_event_to_str(enum uart_event_type type)
{
	switch (type) {
	case UART_TX_DONE:
		return "UART_TX_DONE";
	case UART_TX_ABORTED:
		return "UART_TX_ABORTED";
	case UART_RX_RDY:
		return "UART_RX_RDY";
	case UART_RX_BUF_REQUEST:
		return "UART_RX_BUF_REQUEST";
	case UART_RX_BUF_RELEASED:
		return "UART_RX_BUF_RELEASED";
	case UART_RX_DISABLED:
		return "UART_RX_DISABLED";
	case UART_RX_STOPPED:
		return "UART_RX_STOPPED";
	default:
		return "<UNKNOWN>";
	}
}

static void uart_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	LOG_DBG("%s", uart_event_to_str(evt->type));

	switch (evt->type) {
	case UART_TX_DONE:
		free_frame((ipc_frame_t **)&evt->data);
		k_sem_give(&tx_finish_sem);
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		break;
	case UART_RX_BUF_REQUEST:
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_DISABLED:
		break;
	case UART_RX_STOPPED:
		break;
	}
}

int ipc_initialize(void)
{
	if (device_is_ready(uart_dev) == false) {
		LOG_ERR("IPC UART device not ready = %d", 0);
		return -1;
	}

	uart_callback_set(uart_dev, uart_callback, NULL);

	return 0;
}

/*___________________________________________________________________________*/

void tx_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(tx_thread_id, 0x400, tx_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

K_FIFO_DEFINE(tx_fifo);

static inline void tx_queue_frame(ipc_frame_t *frame)
{
	k_fifo_put(&tx_fifo, frame);
}

static inline uint32_t ipc_frame_crc32(ipc_frame_t *frame)
{
	uint32_t *const start = (uint32_t *)&frame->seq;
	size_t len = sizeof(frame->data) + sizeof(frame->seq); // multiple of 4

	return crc32_ieee_u32(start, len >> 4);
}

static void wrap_ipc_frame(ipc_frame_t *frame, uint32_t sequence_number)
{
	frame->start_delimiter = IPC_START_FRAME_DELIMITER;
	frame->end_delimiter = IPC_END_FRAME_DELIMITER;

	/* set sequence number */
	frame->seq = sequence_number;

	/* final check on data size */
	if (frame->data.size > IPC_MAX_DATA_SIZE) {
		LOG_ERR("IPC frame data size too big = %d", frame->data.size);
		frame->data.size = IPC_MAX_DATA_SIZE;
	}

	/* padding */
	memset(&frame->data.buf[frame->data.size], 0x00,
	       IPC_MAX_DATA_SIZE - frame->data.size);
	
	/* calculate crc32 */
	frame->crc32 = ipc_frame_crc32(frame);
}

void tx_thread(void *_a, void *_b, void *_c)
{
	int ret;
	ipc_frame_t *frame;
	uint32_t seq = 0U;

	ipc_initialize();

	for (;;) {
		frame = k_fifo_get(&tx_fifo, K_FOREVER);
		if (frame == NULL) {
			continue;
		}

		/* prepare IPC frame, sfd, efd, crc32, ... */
		wrap_ipc_frame(frame, seq);

		/* wait for tx finish */
		ret = k_sem_take(&tx_finish_sem, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("k_sem_take failed = %d", ret);
			free_frame(&frame);
			continue;
		}

		/* send frame */
		ret = uart_tx(uart_dev, (const uint8_t *)frame,
			      IPC_FRAME_SIZE, IPC_UART_TX_TIMEOUT_MS);
		if (ret != 0) {
			LOG_ERR("uart_tx failed = %d", ret);

			/* retry on failure */
			tx_queue_frame(frame);
			continue;
		}

		/* increment sequence number */
		seq++;

		/* show frame being sent */
		ipc_log_frame(frame);
	}
}

/*___________________________________________________________________________*/

int ipc_allocate_frame(ipc_frame_t **frame)
{
	return alloc_frame(frame);
}

void ipc_free_frame(ipc_frame_t **frame)
{
	if (*frame != NULL) {
		free_frame(frame);
		*frame = NULL;
	}
}

int ipc_send_frame(ipc_frame_t *frame)
{
	int ret = -EINVAL;

	if (frame != NULL) {
		tx_queue_frame(frame);
		ret = 0;
	}

	return ret;
}

int ipc_send_data(const ipc_data_t *data)
{
	int ret;
	
	/* checks */
	if (data == NULL) {
		return -EINVAL;
	}

	if (data->size > IPC_MAX_DATA_SIZE) {
		LOG_ERR("IPC data size too big = %d, forcing to %u",
			data->size, IPC_MAX_DATA_SIZE);
		return -EINVAL;
	}

	/* allocate */
	ipc_frame_t *frame;
	ret = ipc_allocate_frame(&frame);
	if (ret != 0) {
		return ret;
	}
	
	/* copy data */
	memcpy(frame->data.buf, data->buf, data->size);
	frame->data.size = data->size;

	/* queue frame */
	tx_queue_frame(frame);

	return 0;
}

/*___________________________________________________________________________*/

void ipc_log_frame(const ipc_frame_t *frame)
{
	LOG_INF("IPC frame: %u B, seq = %x, data size = %u, sfd = %x, efd = %x crc32=%x",
		IPC_FRAME_SIZE, frame->seq, frame->data.size, frame->start_delimiter,
		frame->end_delimiter, frame->crc32);
	// LOG_HEXDUMP_DBG(frame->data.buf, frame->data.size, "IPC frame data");
}