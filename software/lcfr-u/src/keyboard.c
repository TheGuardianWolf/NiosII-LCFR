/*
 * keyboard.c
 *
 *  Created on: Mar 25, 2018
 *      Author: chris
 */
#include <stdio.h>
#include "altera_up_avalon_ps2.h"
#include "altera_avalon_pio_regs.h"
#include "altera_up_ps2_keyboard.h"
#include "sys/alt_irq.h"
#include "system.h"
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "frequency_analyzer.h"
#include "keyboard.h"

static alt_up_ps2_dev * ps2KB;
static uint8_t currentType;
static KB_CODE_TYPE decodeMode;
static int32_t decodeStatus;
static char keyInputKeycode = 0;
static char keyInputDecoded;
static uint8_t keyBufferIndex = 0;
static char keyBuffer[KB_KEYBUFFER_SIZE];
static char newKey;
static SemaphoreHandle_t xKeyBufferMutex;
static QueueHandle_t xKeyboardQueue;

//keyDown for ignoring key presses
static bool keyDown = false;

static void ps2_isr(void* ps2Device, uint32_t id) {
    decodeStatus = (int32_t)decode_scancode(ps2Device, &decodeMode, &keyInputKeycode, &keyInputDecoded);
    //check if decoded key input is equal to 0 cause of bug where the ISR is entered multiple times from 1 key press
    if (decodeStatus == 0 && decodeMode != KB_BREAK_CODE) {
        keyDown = !keyDown;
        if (keyDown) {
            switch (decodeMode) {
            case KB_ASCII_MAKE_CODE :
                if ((keyInputDecoded >= '0' && keyInputDecoded <= '9') || (keyInputDecoded == '.')) {
                    xQueueSendFromISR(xKeyboardQueue, &keyInputDecoded, NULL);
                }
                break ;
            case KB_LONG_BINARY_MAKE_CODE :
                break;
            case KB_BINARY_MAKE_CODE :
                if ((keyInputKeycode == KB_TAB) || (keyInputKeycode == KB_BACKSPACE) || (keyInputKeycode == KB_ENTER)) {
                    xQueueSendFromISR(xKeyboardQueue, &keyInputKeycode, NULL);
                }
                break ;
            case KB_BREAK_CODE :
                break;
            default :
                printf ( "DEFAULT   : %x\n", keyInputKeycode );
                printf ( "DEFAULT   : %x\n", keyInputDecoded );
                break ;
            }
        }
    }
}

static void changeType() {
    if (currentType < FREQUENCY_ANALYZER_CONFIG_TYPES) {
        currentType++;
    }
    else {
        currentType = 0;
    }
}

static void Task_KB(void *pvParameters ) {
    while(1) {
        if (xQueueReceive(xKeyboardQueue, &newKey, portMAX_DELAY) == pdTRUE) {
            if ((newKey >= '0' && newKey <= '9' ) || newKey == '.') {
				if (keyBufferIndex < KB_KEYBUFFER_SIZE) {
					KB_setKey(keyBufferIndex, newKey);
                	keyBufferIndex++;
				}
            }
            else if (newKey == KB_ENTER) {
                KB_setKey(keyBufferIndex, '\0');
				KB_getKeyBuffer(newKey);
                FrequencyAnalyzer_setConfig(currentType, (float)atof(newKey));
                KB_setKey(0, '\0');
                keyBufferIndex = 0;
                changeType();
            }
            else if (newKey == KB_BACKSPACE) {
				if (keyBufferIndex > 1) {
					keyBufferIndex--;
				}
				KB_setKey(keyBufferIndex, '\0');
            }
            else if (newKey == KB_TAB) {
                KB_setKey(0, '\0');
                keyBufferIndex = 0;
                changeType();
            }
        }
    }
}

uint8_t KB_getKeyBufferType() {
	return currentType;
}

void KB_getKeyBuffer(char* buf) {
    xSemaphoreTake(xKeyBufferMutex, portMAX_DELAY);
    memcpy(buf, keyBuffer, sizeof(keyBuffer));
    xSemaphoreGive(xKeyBufferMutex);
}

void KB_setKeyBuffer(char* buf) {
    xSemaphoreTake(xKeyBufferMutex, portMAX_DELAY);
    memcpy(keyBuffer, buf, sizeof(keyBuffer));
    xSemaphoreGive(xKeyBufferMutex);
}

void KB_setKey(size_t keyIndex, char k) {
    xSemaphoreTake(xKeyBufferMutex, portMAX_DELAY);
    keyBuffer[keyIndex] = k;
    xSemaphoreGive(xKeyBufferMutex);
}

void KB_start() {
    //enable interrupt for keyboard
    ps2KB = alt_up_ps2_open_dev(PS2_NAME);
    alt_up_ps2_clear_fifo (ps2KB) ;
    alt_irq_register(PS2_IRQ, ps2KB, ps2_isr);
    // register the PS/2 interrupt
    IOWR_8DIRECT(PS2_BASE,4,1);

    xKeyBufferMutex = xSemaphoreCreateMutex();
    //Create queue for display
    xKeyboardQueue = xQueueCreate( 16, sizeof(char));
    //Create draw task
    xTaskCreate( KB_Task, "KB", configMINIMAL_STACK_SIZE, NULL, 4, NULL );    
}
