/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <system.h>

#include "load_switch.h"
#include "user_interface.h"

/* Scheduler includes. */
#include "freertos/FreeRTOS.h"

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {
	init();
	LoadSwitch_createTask();

	/* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;);
}

void init() {
	init_interface();
}
