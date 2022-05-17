#include <zephyr.h>

#include "nrf52840.h"

int main(void)
{
	NRF_P0->PIN_CNF[0x03U] = GPIO_DIR_PIN3_Output | GPIO_PIN_CNF_PULL_Disabled;
	NRF_P0->OUT = GPIO_OUT_PIN3_Msk;

	NRF_P0->PIN_CNF[0x04U] = GPIO_DIR_PIN4_Output | GPIO_PIN_CNF_PULL_Disabled;
	NRF_P0->OUT = GPIO_OUT_PIN4_Msk;

	for (;;) {
		NRF_P0->OUT = GPIO_OUT_PIN3_Msk | GPIO_OUT_PIN4_Msk;
		k_sleep(K_SECONDS(1));

		NRF_P0->OUT &= ~(GPIO_OUT_PIN3_Msk | GPIO_OUT_PIN4_Msk);
		k_sleep(K_SECONDS(1));
	}
}