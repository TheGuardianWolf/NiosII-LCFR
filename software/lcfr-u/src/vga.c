#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include <limits.h>


#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/queue.h"

#include "VGA.h"
#include "keyboard.h"
#include "frequency_analyzer.h"
#include "load_manager.h"
#include "event.h"

static QueueHandle_t xVGAQueue;
static SemaphoreHandle_t xNewConfigValueMutex;
static alt_up_pixel_buffer_dma_dev *pixelBuf;
static alt_up_char_buffer_dev *charBuf;
static const TickType_t xFrequency = VGA_PERIOD * portTICK_PERIOD_MS;
static uint8_t configType = 0;
static char configValues[VGA_CONFIG_TYPES_COUNT][KB_KEYBUFFER_SIZE] = {
	"", "", ""
};
static char newConfigValue[KB_KEYBUFFER_SIZE] = "";
static char timeBuf[KB_KEYBUFFER_SIZE];
static VGAFrequencyInfo frequencyInfo[100];
static VGAFrequencyInfo receivedFrequencyInfo;
static ReactionTimes reactionTimes;

static setupDisplay() {
	//Set up plot axes
	alt_up_pixel_buffer_dma_draw_hline(pixelBuf, 100, 590, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_hline(pixelBuf, 100, 590, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixelBuf, 100, 50, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixelBuf, 100, 220, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);

	alt_up_char_buffer_string(charBuf, "Frequency(Hz)", 4, 4);
	alt_up_char_buffer_string(charBuf, "52", 10, 7);
	alt_up_char_buffer_string(charBuf, "50", 10, 12);
	alt_up_char_buffer_string(charBuf, "48", 10, 17);
	alt_up_char_buffer_string(charBuf, "46", 10, 22);

	alt_up_char_buffer_string(charBuf, "df/dt(Hz/s)", 4, 26);
	alt_up_char_buffer_string(charBuf, "60", 10, 28);
	alt_up_char_buffer_string(charBuf, "30", 10, 30);
	alt_up_char_buffer_string(charBuf, "0", 10, 32);
	alt_up_char_buffer_string(charBuf, "-30", 9, 34);
	alt_up_char_buffer_string(charBuf, "-60", 9, 36);

	alt_up_char_buffer_string(charBuf, "Reaction time(ms): ", 4, 41);
	alt_up_char_buffer_string(charBuf, "Min: ", 4, 43);
	alt_up_char_buffer_string(charBuf, "Max: ", 20, 43);
	alt_up_char_buffer_string(charBuf, "Ave: ", 36, 43);
	alt_up_char_buffer_string(charBuf, "Total: ", 52, 43);

	alt_up_char_buffer_string(charBuf, "System Status: ", 4, 46);
	alt_up_char_buffer_string(charBuf, "Lower Freq Threshold: ", 4, 48);
	alt_up_char_buffer_string(charBuf, "Upper Freq Threshold: ", 4, 50);
	alt_up_char_buffer_string(charBuf, "Change in Freq Threshold: ", 4, 52);

	alt_up_char_buffer_string(charBuf, "Enter New Lower Freq Threshold: ", 38, 48);
	alt_up_char_buffer_string(charBuf, "Enter New Upper Freq Threshold: ", 38, 50);
	alt_up_char_buffer_string(charBuf, "Enter Change in Freq Threshold: ", 38, 52);

	alt_up_pixel_buffer_dma_clear_screen(pixelBuf, 0);
	alt_up_char_buffer_clear(charBuf);
}


static void Task_VGA(void *pvParameters ){
	setupDisplay();
	TickType_t xLastWakeTime;

	int32_t k, i = 99, j = 0;
	VGALine lineFreq, lineRoC;
	
	while(1){
		xLastWakeTime = xTaskGetTickCount();

		configType = KB_getKeyBufferType();
		KB_getKeyBuffer(newConfigValue);
		reactionTimes = LoadManager_getReactionTimes();

		size_t newConfigValueLength = strlen(newConfigValue);
		for (k = newConfigValueLength; k < 6; k++) {
			newConfigValue[k] = ' ';
		}
		if (newConfigValueLength < 6) {
			newConfigValue[newConfigValueLength] = (xLastWakeTime % 500 < 250) ? '_' : ' ';
		}
		newConfigValue[6] = '\0';

		for (k = 0; k < VGA_CONFIG_TYPES_COUNT; k++) {
			snprintf(configValues[k], KB_KEYBUFFER_SIZE, "%6.2f", FrequencyAnalyzer_getConfig(k));
		}

		//receive frequency data from queue
		while (xQueueReceive(xVGAQueue, &receivedFrequencyInfo, 0) == pdTRUE) {
			alt_up_char_buffer_string(charBuf, receivedFrequencyInfo.stable ? "Stable  " : "Unstable", 19, 46);
			frequencyInfo[i] = receivedFrequencyInfo;

			//guards incase the values overflow the graph
			if (frequencyInfo[i].derivative > 60.0f) {
				frequencyInfo[i].derivative = 60.0f;
			}
			else if (frequencyInfo[i].derivative < -60.0f) {
				frequencyInfo[i].derivative = -60.0f;
			}
			if (frequencyInfo[i].instant > 52.0f) {
				frequencyInfo[i].instant = 52.0f;
			}
			else if (frequencyInfo[i].instant < 46.0f) {
				frequencyInfo[i].instant = 46.0f;
			}
			i =	++i%100; //point to the next data (oldest) to be overwritten
		}

		for (k = 0; k < VGA_CONFIG_TYPES_COUNT; k++) {
			alt_up_char_buffer_string(charBuf, configValues[k], 26, 48 + k * 2);
			alt_up_char_buffer_string(charBuf, configType == k ? newConfigValue : "      ", 71, 48 + k * 2);
		}

		for (k = 0; k < VGA_CONFIG_TYPES_COUNT; k++) {
			uint64_t t = ((uint64_t*)(&reactionTimes))[k];
			if (t == UINT_MAX || t == 0.0f) {
				snprintf(timeBuf, sizeof(timeBuf), "----");
			}
			else {
				snprintf(timeBuf, sizeof(timeBuf), "%4.4f", (float)t / 100000);
			}
			alt_up_char_buffer_string(charBuf, timeBuf, 9 + k * 16, 43);
		}

		snprintf(timeBuf, sizeof(timeBuf), "%8u", (unsigned int)(xLastWakeTime * portTICK_PERIOD_MS));
		alt_up_char_buffer_string(charBuf, timeBuf, 59, 43);

		//clear old graph to draw new graph
		alt_up_pixel_buffer_dma_draw_box(pixelBuf, 101, 0, 639, 199, 0, 0);
		alt_up_pixel_buffer_dma_draw_box(pixelBuf, 101, 201, 639, 299, 0, 0);

		for(j=0;j<99;++j){ //i here points to the oldest data, j loops through all the data to be drawn on VGA
			if (((int)(frequencyInfo[(i+j)%100].instant) > MIN_FREQ) && ((int)(frequencyInfo[(i+j+1)%100].instant) > MIN_FREQ)){
				//Calculate coordinates of the two data points to draw a line in between
				//Frequency plot
				lineFreq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
				lineFreq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (frequencyInfo[(i+j)%100].instant - MIN_FREQ));

				lineFreq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
				lineFreq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (frequencyInfo[(i+j+1)%100].instant - MIN_FREQ));

				//Frequency RoC plot
				lineRoC.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
				lineRoC.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * frequencyInfo[(i+j)%100].derivative);

				lineRoC.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
				lineRoC.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * frequencyInfo[(i+j+1)%100].derivative);

				//Draw
				alt_up_pixel_buffer_dma_draw_line(pixelBuf, lineFreq.x1, lineFreq.y1, lineFreq.x2, lineFreq.y2, 0x3ff << 0, 0);
				alt_up_pixel_buffer_dma_draw_line(pixelBuf, lineRoC.x1, lineRoC.y1, lineRoC.x2, lineRoC.y2, 0x3ff << 0, 0);
			}
		}

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

QueueHandle_t VGA_getQueueHandle() {
	return xVGAQueue;
}

void VGA_start(){
	//initialize VGA controllers
	pixelBuf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
	if(pixelBuf == NULL){
		printf("can't find pixel buffer device\n");
	}
	alt_up_pixel_buffer_dma_clear_screen(pixelBuf, 1);

	charBuf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
	if(charBuf == NULL){
		printf("can't find char buffer device\n");
	}
	alt_up_char_buffer_clear(charBuf);

	memset(frequencyInfo, 0, sizeof(frequencyInfo));

	xNewConfigValueMutex = xSemaphoreCreateMutex();

	//Create queue for display
	xVGAQueue = xQueueCreate( 16, sizeof(VGAFrequencyInfo));

	//Create draw task
	xTaskCreate(Task_VGA, "DrawTsk", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
}
