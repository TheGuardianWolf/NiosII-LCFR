#include <stdio.h>
#include <altera_avalon_pio_regs.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "switch.h"
#include "load_manager.h"
#include "system.h"
#include "event.h"

static const TickType_t xFrequency = SWITCH_PERIOD * portTICK_PERIOD_MS;
static SemaphoreHandle_t xStateMutex;

static bool state[5] = {true, true, true, true, true};

static void Task_switch(void *pvParameters) {
	TickType_t xLastWakeTime;
	bool newState;
	Event event;
	uint8_t switchValue;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		event.timestamp = timestamp();

		uint8_t i;
		bool newStates[5];
		xSemaphoreTake(xStateMutex, portMAX_DELAY);
		for (i = 0; i < SWITCH_COUNT; i++) {
			newState = (switchValue >> i) & 1;
			if (newState != state[i]) {
				event.code = state[i] ? EVENT_SWITCH_OFF(i) : EVENT_SWITCH_ON(i);	
				xQueueSend(LoadManager_getQueueHandle(), &event, portMAX_DELAY);
			}
			newStates[i] = newState;
		}
		memcpy(state, newStates, sizeof(state));
		xSemaphoreGive(xStateMutex);


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
	xStateMutex = xSemaphoreCreateMutex();

	xTaskCreate(Task_switch, "switch", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
}

void Switch_getState(bool * buf) {
	xSemaphoreTake(xStateMutex, portMAX_DELAY);
	memcpy(buf, state, sizeof(state));
	xSemaphoreGive(xStateMutex);
	printf("Current switch state is [%u, %u, %u, %u, %u]\n",
			state[0], state[1], state[2], state[3], state[4]);
}
