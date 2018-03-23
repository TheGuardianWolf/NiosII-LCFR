#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdio.h>

void Task_frequencyAnalyzer(void *pvParameters)
{
	while (1) {
		printf("Task 2\n");
		vTaskDelay(1000);
	}
}
