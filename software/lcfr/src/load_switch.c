#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "load_switch.h"

static const TickType_t xFrequency = 50 * portTICK_PERIOD_MS;

static bool loadState[5] = {true, true, true, true, true};

static void Task_loadSwitch(void *pvParameters) {
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		uint8_t i;
		for (i = 0; i < LOAD_SWITCH_MAX; i++) {
			loadState[i] = (switchValue >> i) & 1;
		}

#if DEBUG == 1
		printf("Task_loadSwitch ran at %u\n", xLastWakeTime);
		printf("Current switch state is [%u, %u, %u, %u, %u]",
				loadState[0], loadState[1], loadState[2], loadState[3], loadState[4]);
#endif

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void LoadSwitch_start() {
	xTaskCreate(Task_loadSwitch, "LoadSwitch", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

bool LoadSwitch_getState(uint8_t i) {
	return loadState[i];
}
