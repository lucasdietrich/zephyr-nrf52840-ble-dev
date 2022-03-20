#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "ipc_uart/ipc.h"

const char * strings[] = {
	"Hello World",

	// long text
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. ",
	"Sed ut perspiciatis unde omnis iste natus error sit voluptatem ",
	"accusantium doloremque laudantium, totam rem aperiam, eaque ipsa ",
	"quae ab illo inventore veritatis et quasi architecto beatae ",
	"vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia ",
	"voluptas sit aspernatur aut odit aut fugit, sed quia ",
	"consequuntur magni dolores eos qui ratione voluptatem ",
	"sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum ",
};

int main(void)
{
	uint32_t i = 0U;
	ipc_frame_t *frame;

	for (;;) {
		if (ipc_allocate_frame(&frame) == 0) {
			frame->data.size = strlen(strings[i]);
			memcpy(frame->data.buf, strings[i], frame->data.size);
			ipc_send_frame(frame);

			i = (i + 1) % ARRAY_SIZE(strings);
		}

		k_sleep(K_MSEC(200));
	}
	return 0;
}