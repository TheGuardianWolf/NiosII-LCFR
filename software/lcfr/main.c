/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <system.h>

/* Scheduler includes. */
#include "freertos/FreeRTOS.h"

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {
	/* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;);
}
