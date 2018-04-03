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
#include "altera_up_ps2_keyboard.h"
#include "sys/alt_irq.h"
#include "system.h"

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "config.h"

static enum config_type current_type;
static KB_CODE_TYPE decode_mode;
static int decode_status;
static unsigned char keyInput_keycode = 0;
static char keyInput_decoded;
static char keyBufferVGA;
static struct config config_info;
static char keyBuffer[16];
//maybe deadlock caused by these 2 semaphores??
static SemaphoreHandle_t shared_keys_sem;
static SemaphoreHandle_t shared_config_sem;
static QueueHandle_t xKeyboardQueue;

static void ps2_isr(void* ps2_device, alt_u32 id){
	decode_status = decode_scancode (ps2_device, &decode_mode , &keyInput_keycode , &keyInput_decoded);
	printf("status: %d\n", decode_status);
	//check if decoded key input is equal to 0 cause of bug where the ISR is entered multiple times from 1 key press
	if (decode_status == 0 && keyInput_decoded != 0) {
		if ((keyInput_decoded >= '0' && keyInput_decoded <= '9') || (keyInput_decoded == 9) || (keyInput_decoded == 10)) {
			xQueueSendFromISR(xKeyboardQueue, &keyInput_decoded, NULL);
		}
        // print out the result
//        switch ( decode_mode )
//        {
//          case KB_ASCII_MAKE_CODE :
//            printf ( "ASCII   : %x\n", keyInput_keycode ) ;
//            break ;
//          case KB_LONG_BINARY_MAKE_CODE :
//            // do nothing
//          case KB_BINARY_MAKE_CODE :
//            printf ( "MAKE CODE : %x\n", keyInput_keycode ) ;
//            break ;
//          case KB_BREAK_CODE :
//            // do nothing
//          default :
//            printf ( "DEFAULT   : %x\n", keyInput_keycode ) ;
//            break ;
//        }
	}
#if DEBUG == 0
	printf("Inputted: %x \n",keyInput_decoded);
#endif
}

void KB_Task(void *pvParameters ) {
	char keyBufferTemp;
	unsigned int i;
	struct config config_info_temp;
	while(1) {
		if(xQueueReceive(xKeyboardQueue, &keyBufferTemp, portMAX_DELAY) == pdTRUE) {
			if(xSemaphoreTake(shared_config_sem, portMAX_DELAY) == pdTRUE) {
				keyBufferVGA = keyBufferTemp;
				printf("Received: %c \n",keyBufferTemp);
				xSemaphoreGive(shared_keys_sem);
			}

			if (keyBufferTemp >= '0' && keyBufferTemp <= '9') {
				keyBuffer[i] = keyBufferTemp - '0';
				printf("Char: %d\n", keyBuffer[i]);
				i++;
			}
			else if (keyBufferTemp == 10){
				int j;
				for (j = 0; j < i + 1; j++) {
					config_info_temp.value += keyBuffer[j] * ((i - j + 1) * 10);
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

char getKey() {
	return keyBufferVGA;
}

struct config getConfig() {
	return config_info;
}

SemaphoreHandle_t getKeySemaphore() {
	return shared_keys_sem;
}

SemaphoreHandle_t getConfigSemaphore() {
	return shared_config_sem;
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
	alt_up_ps2_clear_fifo (ps2_kb) ;

	alt_irq_register(PS2_IRQ, ps2_kb, ps2_isr);
	// register the PS/2 interrupt
	IOWR_8DIRECT(PS2_BASE,4,1);
	//Create draw task
	xTaskCreate( KB_Task, "KBTsk", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	//Create queue for display
	xKeyboardQueue = xQueueCreate( 16, sizeof(char));

	//create the counting semaphore for the keys variable
	shared_keys_sem = xSemaphoreCreateCounting( 9999, 1 );

	//create the counting semaphore for the config variable
	shared_config_sem = xSemaphoreCreateCounting( 9999, 1 );

	current_type = lower_freq;

	printf("finished KB init");
}
