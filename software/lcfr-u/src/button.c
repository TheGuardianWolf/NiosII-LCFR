#include <stdio.h>
#include <stdint.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "button.h"
#include "load_manager.h"
#include "system.h"
#include "event.h"

static void ISR_button() {
	uint8_t buttonValue = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);

	if ((buttonValue & 1) == 1) {
		Event event = {
			.code = EVENT_BUTTON_PRESSED,
			.timestamp = timestampFromISR()
		};
		xQueueSendFromISR(LoadManager_getQueueHandle(), &event, NULL);
	}
}

void Button_start() {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0x7);
	alt_irq_register(PUSH_BUTTON_IRQ, NULL, ISR_button);
}
