/*
 * keyboard.c
 *
 *  Created on: Mar 25, 2018
 *      Author: chris
 */
#include <stdio.h>
#include <stdbool.h>
#include "altera_up_avalon_ps2.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "system.h"

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "config.h"

static enum config_type current_type;
static unsigned char keyInput;
static unsigned char keyBufferVGA;
static struct config config_info;
static unsigned char keyBuffer[16];
//maybe deadlock caused by these 2 semaphores??
static SemaphoreHandle_t shared_keys_sem;
static SemaphoreHandle_t shared_config_sem;
static QueueHandle_t xKeyboardQueue;

static void ps2_isr(void* ps2_device, alt_u32 id){
	alt_up_ps2_read_data_byte_timeout(ps2_device, &keyInput);
	if ((keyInput >= '0' && keyInput <= '9') || (keyInput == 9) || (keyInput == 10)) {
		xQueueSendFromISR(xKeyboardQueue, &keyInput, NULL);
	}
#if DEBUG == 1
	printf("Scan code: %x\n", keyInput);
#endif
}

void KB_Task(void *pvParameters ) {
	unsigned char keyBufferTemp;
	unsigned int i;
	struct config config_info_temp;
	while(1) {
		if(xQueueReceive(xKeyboardQueue, &keyBufferTemp, portMAX_DELAY) == pdTRUE) {
			if(xSemaphoreTake(shared_config_sem, portMAX_DELAY) == pdTRUE) {
				keyBufferVGA = keyBufferTemp;
				xSemaphoreGive(shared_keys_sem);
			}

			if (keyBufferTemp >= '0' && keyBufferTemp <= '9') {
				keyBuffer[i] = keyBufferTemp - '0';
				i++;
			}
			else if (keyBufferTemp == 10){
				int j;
				for (j = 0; j < i + 1; j++) {
					config_info_temp.value += keyBuffer[j] * ((i - j + 1) * 10)
				}
				config_info_temp.type = current_type;

				if(xSemaphoreTake(shared_config_sem, portMAX_DELAY) == pdTRUE) {
					config_info = config_info_temp;
					xSemaphoreGive(shared_config_sem);
				}

				i = 0;
			}
			else {
				int j;
				change_type();
				for (j = 0; j < i + 1; j++) {
					keyBuffer[j] = 0;
				}
				i = 0;
			}
		}
	}
}

QueueHandle_t KB_getQueueHandle() {
	return xKeyboardQueue;
}

unsigned char getKey() {
	return keyBufferVGA;
}

SemaphoreHandle_t getKeySemaphore() {
	return shared_keys_sem;
}

void change_type() {
	if (current_type == lower_freq) {
		current_type = change_in_freq;
	}
	else {
		current_type = lower_freq;
	}
}

void KB_start(){
	//enable interrupt for keyboard
	alt_up_ps2_dev * ps2_kb = alt_up_ps2_open_dev(PS2_NAME);
	alt_up_ps2_enable_read_interrupt(ps2_kb);
	alt_irq_register(PS2_IRQ, ps2_kb, ps2_isr);

	//Create draw task
	xTaskCreate( KB_Task, "KBTsk", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	//Create queue for display
	xKeyboardQueue = xQueueCreate( 16, sizeof(unsigned char));

	//create the counting semaphore for the keys variable
	shared_keys_sem = xSemaphoreCreateCounting( 9999, 1 );

	//create the counting semaphore for the config variable
	shared_config_sem = xSemaphoreCreateCounting( 9999, 1 );

	current_type = lower_freq;
}
