#include <zephyr.h>

int main(void)
{
	for (;;) {
		k_sleep(K_SECONDS(1));
	}
}