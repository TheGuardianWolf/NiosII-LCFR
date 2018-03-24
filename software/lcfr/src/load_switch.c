#include <stdbool.h>
#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system.h"

#include "load_switch.h"

static const TickType_t xFrequency = 50 * portTICK_PERIOD_MS;

static bool loadStates[5] = {true, true, true, true, true};

static void Task_loadSwitch(void *pvParameters) {
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		uint8_t i;
		for (i = 0; i < LOAD_SWITCH_MAX; i++) {
			loadStates[i] = (switchValue >> i) & 1;
			if (i == LOAD_SWITCH_MAX - 1){
				printf("%d\n",loadStates[i]);
			}
			else {
				printf("%d",loadStates[i]);
			}
		}

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void LoadSwitch_createTask() {
	xTaskCreate(Task_loadSwitch, "LoadSwitch", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

bool LoadSwitch_getState(uint8_t i) {
	return loadStates[i];
}
