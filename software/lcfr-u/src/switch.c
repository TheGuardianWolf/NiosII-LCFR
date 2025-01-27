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
//static SemaphoreHandle_t xStateMutex;

static bool state[5] = {true, true, true, true, true};

static void Task_switch(void *pvParameters) {
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t i;
		bool prevState;
		Event event;
		uint8_t switchValue;	

		switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

		event.timestamp = timestamp();

		for (i = 0; i < SWITCH_COUNT; i++) {
			prevState = Switch_getState(i);
			state[i] = (switchValue >> i) & 1;
			if (prevState != Switch_getState(i)) {
				event.code = prevState ? EVENT_SWITCH_OFF(i) : EVENT_SWITCH_ON(i);	
				xQueueSend(LoadManager_getQueueHandle(), &event, 0);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void Switch_start() {
	uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
	uint8_t i;
	for (i = 0; i < SWITCH_COUNT; i++) {
		state[i] = (switchValue >> i) & 1;
	}
	xTaskCreate(Task_switch, "switch", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

bool Switch_getState(uint8_t i) {
	return state[i];
}
