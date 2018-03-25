#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "switch.h"

static const TickType_t xFrequency = SWITCH_PERIOD * portTICK_PERIOD_MS;

static bool state[5] = {true, true, true, true, true};

static void Task_switch(void *pvParameters) {
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		uint8_t i;
		for (i = 0; i < SWITCH_COUNT; i++) {
			state[i] = (switchValue >> i) & 1;
		}

#if DEBUG == 1
		printf("Task_loadSwitch ran at %u\n", xLastWakeTime);
		printf("Current switch state is [%u, %u, %u, %u, %u]",
				loadState[0], loadState[1], loadState[2], loadState[3], loadState[4]);
#endif

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void Switch_start() {
	xTaskCreate(Task_switch, "switch", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

bool Switch_getState(uint8_t i) {
	return state[i];
}
