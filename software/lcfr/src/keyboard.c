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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "config.h"
#include "frequency_analyzer.h"
#include "keyboard.h"

static alt_up_ps2_dev * ps2_kb;
static enum config_type current_type;
static KB_CODE_TYPE decode_mode;
static int decode_status;
static char keyInput_keycode = 0;
static char keyInput_decoded;
static char keyBuffer[KB_KEYBUFFER_SIZE];
static SemaphoreHandle_t xKeyBufferSemaphore;
static QueueHandle_t xKeyboardQueue;
TaskHandle_t xHandle;

//flag for ignoring key presses
static bool flag = false;

static void ps2_isr(void* ps2_device, alt_u32 id){
	decode_status = decode_scancode (ps2_device, &decode_mode , &keyInput_keycode , &keyInput_decoded);
	//check if decoded key input is equal to 0 cause of bug where the ISR is entered multiple times from 1 key press
	if (decode_status == 0 && decode_mode != KB_BREAK_CODE) {
        flag = !flag;
        //print out the result
        switch ( decode_mode )
        {
          case KB_ASCII_MAKE_CODE :
            if(flag == true)
            {
            	if ((keyInput_decoded >= '0' && keyInput_decoded <= '9') || (keyInput_keycode == 0xd) || (keyInput_keycode == 0x5a)) {
            		xQueueSendFromISR(xKeyboardQueue, &keyInput_decoded, NULL);
            	}
            	break ;
            }
          case KB_LONG_BINARY_MAKE_CODE :
        	  break;
            // do nothing
          case KB_BINARY_MAKE_CODE :
        	  if(flag == true) {
        		  if ((keyInput_keycode == 0xd) || (keyInput_keycode == 0x5a)) {
        			xQueueSendFromISR(xKeyboardQueue, &keyInput_keycode, NULL);
        		  }
        	  }
            break ;
          case KB_BREAK_CODE :
              break;
          default :
            printf ( "DEFAULT   : %x\n", keyInput_keycode );
            printf ( "DEFAULT   : %x\n", keyInput_decoded );
            break ;
        }
	}
#if DEBUG == 1
	printf("Inputted: %x \n",keyInput_decoded);
#endif
}

static void change_type() {
	if (current_type == lower_freq) {
		current_type = change_in_freq;
	}
	else {
		current_type = lower_freq;
	}
}

void KB_Task(void *pvParameters ) {
	xHandle = xTaskGetCurrentTaskHandle();
	char keyBufferTemp;
	unsigned int i = 0;
	struct config config_info_temp = {
		.type = current_type,
		.value = 0.0f
	};
	while(1) {
		//printf("%d\n",(int)uxQueueSpacesAvailable( xKeyboardQueue ));
		if(xQueueReceive(xKeyboardQueue, &keyBufferTemp, portMAX_DELAY) == pdTRUE) {
			if (keyBufferTemp >= '0' && keyBufferTemp <= '9') {
				printf("Char: %d\n", keyBufferTemp - '0');
				xSemaphoreTake(xKeyBufferSemaphore, portMAX_DELAY);
				keyBuffer[i] = keyBufferTemp;
				xSemaphoreGive(xKeyBufferSemaphore);
				KB_setKey(i, keyBufferTemp);
				printf("Char: %d\n", keyBuffer[i] - '0');
				i++;
			}
			else if (keyBufferTemp == 0x5a){
				KB_setKey(i, '\0');
				printf("i: %d\n",i);
				config_info_temp.value = (float)atof(keyBuffer);
				config_info_temp.type = current_type;
				printf("%f\n", config_info_temp.value);

				FrequencyAnalyzer_setConfig(config_info_temp.type, config_info_temp.value);

				i = 0;
			}
			else {
				int j;
				change_type();
				for (j = 0; j < i + 1; j++) {
					keyBuffer[j] = 0;
				}
				i = 0;
				printf("tab?\n");
			}
		}
	}
}

void KB_getKeyBuffer(char* buf) {
	xSemaphoreTake(xKeyBufferSemaphore, portMAX_DELAY);
	memcpy(buf, keyBuffer, sizeof(keyBuffer));
	xSemaphoreGive(xKeyBufferSemaphore);
}

void KB_setKeyBuffer(char* buf) {
	xSemaphoreTake(xKeyBufferSemaphore, portMAX_DELAY);
	memcpy(keyBuffer, buf, sizeof(keyBuffer));
	xSemaphoreGive(xKeyBufferSemaphore);
}

void KB_setKey(size_t keyIndex, char k) {
	xSemaphoreTake(xKeyBufferSemaphore, portMAX_DELAY);
	keyBuffer[keyIndex] = k;
	xSemaphoreGive(xKeyBufferSemaphore);
}

void KB_start(){
	//enable interrupt for keyboard
	ps2_kb = alt_up_ps2_open_dev(PS2_NAME);
	alt_up_ps2_clear_fifo (ps2_kb) ;

	alt_irq_register(PS2_IRQ, ps2_kb, ps2_isr);
	// register the PS/2 interrupt
	IOWR_8DIRECT(PS2_BASE,4,1);

	//Create draw task
	xTaskCreate( KB_Task, "KBTsk", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	//Create queue for display
	xKeyboardQueue = xQueueCreate( 16, sizeof(char));

	//create the binary semaphore for the keys variable
	xKeyBufferSemaphore = xSemaphoreCreateMutex();

	current_type = lower_freq;
}
