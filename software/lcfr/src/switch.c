#include <stdio.h>
#include <altera_avalon_pio_regs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "switch.h"
#include "load_manager.h"
#include "system.h"

static const TickType_t xFrequency = SWITCH_PERIOD * portTICK_PERIOD_MS;

static bool state[5] = {true, true, true, true, true};

static void Task_switch(void *pvParameters) {
	printf("enter switch");
	TickType_t xLastWakeTime;
	bool newState;
	uint8_t event;
	uint8_t switchValue;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		uint8_t i;
		for (i = 0; i < SWITCH_COUNT; i++) {
			newState = (switchValue >> i) & 1;
			if (newState != state[i]) {
				event = state[i] ? EVENT_SWITCH_OFF(i) : EVENT_SWITCH_ON(i);
				xQueueSend(LoadManager_getQueueHandle(), &event, 10);
			}
			state[i] = newState;
		}

#if DEBUG == 1
		printf("Task_loadSwitch ran at %u\n", xLastWakeTime);
		printf("Current switch state is [%u, %u, %u, %u, %u]\n",
				state[0], state[1], state[2], state[3], state[4]);
#endif

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void Switch_start() {
	uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
	uint8_t i;
	for (i = 0; i < SWITCH_COUNT; i++) {
		state[i] = (switchValue >> i) & 1;
	}

	xTaskCreate(Task_switch, "switch", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
}

bool Switch_getState(uint8_t i) {
	return state[i];
}
