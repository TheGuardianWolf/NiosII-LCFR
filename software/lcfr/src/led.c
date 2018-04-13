#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "load_manager.h"
#include "system.h"

static const TickType_t xFrequency = LED_PERIOD * portTICK_PERIOD_MS;

static void Task_LED(void *pvParameters) {
	printf("enter led");
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t redLEDValue = 0;
		uint8_t greenLEDValue = 0;

		uint8_t i;
		ManagedState loadState;
		for (i = 0; i < LED_COUNT; i++) {
			loadState = LoadManager_getState(i);
			switch (loadState) {
				case ENABLED:
					redLEDValue |= (0x1 << i);
					break;
				case SHED:
					greenLEDValue |= (0x1 << i);
					break;
				default:
					break;
			}
		}

		IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLEDValue);
		IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, greenLEDValue);

#if DEBUG == 1
		printf("Task_LED ran at %u\n", xLastWakeTime);
#endif

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void LED_start() {
	xTaskCreate(Task_LED, "led", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}
