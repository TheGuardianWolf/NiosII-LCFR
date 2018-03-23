#include "freertos/FreeRTOS.h"

void Task_loadSwitch(void *pvParameters) {
	while (1) {
		printf("Task 2\n");
		vTaskDelay(1000);
	}
}
