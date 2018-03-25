#define DEBUG 1

/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <system.h>

#include "keyboard.h"
#include "vga.h"

/* Scheduler includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void init();

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {
	Switch_start();
	Button_start();
	LED_start();
	LoadManager_start();

	/* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;);
}

void init() {
	VGA_start();
	KB_start();
}
