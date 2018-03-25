#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "button.h"

static ButtonEventHandler eventHandler[BUTTON_COUNT] = {NULL};

static void ISR_button() {
	uint8_t buttonValue = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);

	uint8_t i;
	for (i = 0; i < BUTTON_COUNT; i++) {
		if (!((buttonValue >> i) & 1)) {
			if (eventHandler[i] != NULL) {
				eventHandler[i]();
			}
		}
	}
}

void Button_start() {
	alt_irq_register(PUSH_BUTTON_IRQ, NULL, ISR_button);
}

void Button_ISRAttachHook(uint8_t i, ButtonEventHandler handler) {
	eventHandler[i] = handler;
}

void Button_ISRDetatchHook(uint8_t i) {
	eventHandler[i] = NULL;
}
