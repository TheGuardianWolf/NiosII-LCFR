#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "button.h"
#include "load_manager.h"

static void ISR_button() {
	uint8_t buttonValue = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);

	xQueueSendFromISR(LoadManager_getQueueHandle, EVENT_BUTTON_PRESSED, NULL);
}

void Button_start() {
	alt_irq_register(PUSH_BUTTON_IRQ, NULL, ISR_button);
}
