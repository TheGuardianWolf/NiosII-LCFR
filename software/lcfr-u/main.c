#define DEBUG 1
#include "stdio.h"
/* Scheduler includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/alt_timestamp.h"

#include "button.h"
#include "led.h"
#include "frequency_analyzer.h"
#include "load_manager.h"
#include "switch.h"
#include "keyboard.h"
#include "vga.h"
#include "event.h"

static void init() {
	Event_start();
	VGA_start();
	KB_start();
	Switch_start();
	Button_start();
	LED_start();
	FrequencyAnalyzer_start();
	LoadManager_start();
}

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {
	init();
//	alt_timestamp_start();
	/* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	 the scheduler. */
	for (;;);
}
