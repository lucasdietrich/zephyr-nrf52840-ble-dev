#ifndef _IPC_H_	
#define _IPC_H_

#include <kernel.h>

#include "ipc_frame.h"

/**
 * @brief Send data over IPC UART.
 * 
 * @retval -EINVAL  Invalid argument (data NULL or size > 255).
 * @retval 0	    If successful, negative errno code otherwise.
 */
int ipc_send_data(const ipc_data_t *data);

/**
 * @brief Allocate frame for IPC communication.
 * 
 * @param frame 
 * @return int 
 */
int ipc_allocate_frame(ipc_frame_t **frame);

/**
 * @brief Send the allocated frame.
 * 
 * @param frame 
 * @return int 
 */
int ipc_send_frame(ipc_frame_t *frame);

/**
 * @brief Free buffer for a frame that was not sent.
 * 
 * @param frame 
 */
void ipc_free_frame(ipc_frame_t **frame);

#endif /* _IPC_H_ */