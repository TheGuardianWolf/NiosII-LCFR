#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdio.h>

static bool managed = true;

void Task_loadManager(void *pvParameters) {
	while (1) {
		printf("Task 1\n");
		vTaskDelay(1000);
	}
}
