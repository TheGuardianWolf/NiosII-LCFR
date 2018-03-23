#include "freertos/FreeRTOS.h"

void Task_loadManager(void *pvParameters) {
	while (1) {
		printf("Task 1\n");
		vTaskDelay(1000);
	}
}
