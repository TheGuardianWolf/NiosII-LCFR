#include <stdio.h>
#include <stdlib.h>
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

//For frequency plot
#define FREQPLT_ORI_X 101		//x axis pixel position at the plot origin
#define FREQPLT_GRID_SIZE_X 5	//pixel separation in the x axis between two data points
#define FREQPLT_ORI_Y 199.0		//y axis pixel position at the plot origin
#define FREQPLT_FREQ_RES 20.0	//number of pixels per Hz (y axis scale)

#define ROCPLT_ORI_X 101
#define ROCPLT_GRID_SIZE_X 5
#define ROCPLT_ORI_Y 259.0
#define ROCPLT_ROC_RES 0.5		//number of pixels per Hz/s (y axis scale)

#define MIN_FREQ 45.0 //minimum frequency to draw

#define PRVGADraw_Task_P      (tskIDLE_PRIORITY+1)
TaskHandle_t PRVGADraw;

static QueueHandle_t xVGAQueue;

typedef struct{
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
}Line;

static SemaphoreHandle_t xNewConfigValueMutex;
static alt_up_pixel_buffer_dma_dev *pixel_buf;
static alt_up_char_buffer_dev *char_buf;
static const TickType_t xFrequency = VGA_PERIOD * portTICK_PERIOD_MS;
static size_t configType = 0;
static char configValues[VGA_CONFIG_TYPES_COUNT][KB_KEYBUFFER_SIZE] = {
	"45.0", "55.0", "10.0"
};
static char newConfigValue[KB_KEYBUFFER_SIZE] = "";
static VGAFrequencyInfo frequencyInfo[100];
static VGAFrequencyInfo receivedFrequencyInfo;

/****** VGA display ******/

void PRVGADraw_Task(void *pvParameters ){
	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);
	alt_up_char_buffer_clear(char_buf);
	ReactionTimes reactionTimes;

	//Set up plot axes
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 50, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 220, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);

	alt_up_char_buffer_string(char_buf, "Frequency(Hz)", 4, 4);
	alt_up_char_buffer_string(char_buf, "52", 10, 7);
	alt_up_char_buffer_string(char_buf, "50", 10, 12);
	alt_up_char_buffer_string(char_buf, "48", 10, 17);
	alt_up_char_buffer_string(char_buf, "46", 10, 22);

	alt_up_char_buffer_string(char_buf, "df/dt(Hz/s)", 4, 26);
	alt_up_char_buffer_string(char_buf, "60", 10, 28);
	alt_up_char_buffer_string(char_buf, "30", 10, 30);
	alt_up_char_buffer_string(char_buf, "0", 10, 32);
	alt_up_char_buffer_string(char_buf, "-30", 9, 34);
	alt_up_char_buffer_string(char_buf, "-60", 9, 36);

	alt_up_char_buffer_string(char_buf, "Reaction time(ms): ", 4, 41);
	alt_up_char_buffer_string(char_buf, "Min: ", 4, 43);
	alt_up_char_buffer_string(char_buf, "Max: ", 20, 43);
	alt_up_char_buffer_string(char_buf, "Ave: ", 36, 43);
	alt_up_char_buffer_string(char_buf, "Total: ", 52, 43);

	alt_up_char_buffer_string(char_buf, "System Status: ", 4, 46);
	alt_up_char_buffer_string(char_buf, "Lower Freq Threshold: ", 4, 48);
	alt_up_char_buffer_string(char_buf, "Upper Freq Threshold: ", 4, 50);
	alt_up_char_buffer_string(char_buf, "Change in Freq Threshold: ", 4, 52);

	alt_up_char_buffer_string(char_buf, "Enter New Lower Freq Threshold: ", 38, 48);
	alt_up_char_buffer_string(char_buf, "Enter New Upper Freq Threshold: ", 38, 50);
	alt_up_char_buffer_string(char_buf, "Enter Change in Freq Threshold: ", 38, 52);

	int i = 99, j = 0;
	Line line_freq, line_roc;

	TickType_t xLastWakeTime;
	int k;
	while(1){
		xLastWakeTime = xTaskGetTickCount();

		xSemaphoreTake(xNewConfigValueMutex, portMAX_DELAY);
		KB_getKeyBuffer(newConfigValue);
		size_t newConfigValueLength = strlen(newConfigValue);
		for (k = newConfigValueLength; k < 6; k++) {
			newConfigValue[k] = ' ';
		}
		if (newConfigValueLength < 6) {
			newConfigValue[newConfigValueLength] = (xLastWakeTime % 500 < 250) ? '_' : ' ';
		}
		newConfigValue[6] = '\0';
		xSemaphoreGive(xNewConfigValueMutex);


		for (k = 0; k < VGA_CONFIG_TYPES_COUNT; k++) {
			snprintf(configValues[k], KB_KEYBUFFER_SIZE, "%6.2f", FrequencyAnalyzer_getConfig(k));
		}

		//receive frequency data from queue
		while (xQueueReceive(xVGAQueue, &receivedFrequencyInfo, 0) == pdTRUE) {
			alt_up_char_buffer_string(char_buf, receivedFrequencyInfo.stable ? "Stable  " : "Unstable", 19, 46);
			frequencyInfo[i] = receivedFrequencyInfo;

			//guards incase the values overflow the graph
			if (frequencyInfo[i].derivative > 60.0f) {
				frequencyInfo[i].derivative = 60.0f;
			}
			else if (frequencyInfo[i].derivative < -60.0f) {
				frequencyInfo[i].derivative = -60.0f;
			}
			if (frequencyInfo[i].freq > 52.0f) {
				frequencyInfo[i].freq = 52.0f;
			}
			else if (frequencyInfo[i].freq < 46.0f) {
				frequencyInfo[i].freq = 46.0f;
			}
			i =	++i%100; //point to the next data (oldest) to be overwritten
		}

		reactionTimes = LoadManager_getReactionTimes();

		alt_up_char_buffer_string(char_buf, configValues[0], 26, 48);
		alt_up_char_buffer_string(char_buf, configValues[1], 26, 50);
		alt_up_char_buffer_string(char_buf, configValues[2], 30, 52);
		alt_up_char_buffer_string(char_buf, configType == 0 ? newConfigValue : "     ", 71, 48);
		alt_up_char_buffer_string(char_buf, configType == 1 ? newConfigValue : "     ", 71, 50);
		alt_up_char_buffer_string(char_buf, configType == 2 ? newConfigValue : "     ", 71, 52);

		char sysTime[KB_KEYBUFFER_SIZE];
		for (k = 0; k < 3; k++) {
			uint32_t t = (((uint32_t*)(&reactionTimes))[k]);
			if (t == UINT_MAX || t == 0) {
				snprintf(sysTime, sizeof(sysTime), "--------");
			}
			else {
				snprintf(sysTime, sizeof(sysTime), "%8u", (unsigned int)t);
			}
			alt_up_char_buffer_string(char_buf, sysTime, 9 + k * 16, 43);
		}

//		alt_up_char_buffer_string(char_buf, sysTime, 9, 43);
//		snprintf(sysTime, sizeof(sysTime), "%u", (reactionTimes.min));
//		printf("min: %s\n", sysTime);
//		alt_up_char_buffer_string(char_buf, sysTime, 25, 43);
//		snprintf(sysTime, sizeof(sysTime), "%u", (reactionTimes.average));
//		alt_up_char_buffer_string(char_buf, sysTime, 41, 43);
		snprintf(sysTime, sizeof(sysTime), "%12llu", timestamp()/*(xLastWakeTime * portTICK_PERIOD_MS)*/);
		alt_up_char_buffer_string(char_buf, sysTime, 59, 43);

		//clear old graph to draw new graph
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 0, 639, 199, 0, 0);
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 201, 639, 299, 0, 0);

		for(j=0;j<99;++j){ //i here points to the oldest data, j loops through all the data to be drawn on VGA
			if (((int)(frequencyInfo[(i+j)%100].freq) > MIN_FREQ) && ((int)(frequencyInfo[(i+j+1)%100].freq) > MIN_FREQ)){
				//Calculate coordinates of the two data points to draw a line in between
				//Frequency plot
				line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
				line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (frequencyInfo[(i+j)%100].freq - MIN_FREQ));

				line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
				line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (frequencyInfo[(i+j+1)%100].freq - MIN_FREQ));

				//Frequency RoC plot
				line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
				line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * frequencyInfo[(i+j)%100].derivative);

				line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
				line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * frequencyInfo[(i+j+1)%100].derivative);

				//Draw
				alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_freq.x1, line_freq.y1, line_freq.x2, line_freq.y2, 0x3ff << 0, 0);
				alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_roc.x1, line_roc.y1, line_roc.x2, line_roc.y2, 0x3ff << 0, 0);
			}
		}
		//delay for 16.6 which is close enough to 17 for 60Hz screen
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

QueueHandle_t VGA_getQueueHandle() {
	return xVGAQueue;
}

void VGA_start(){
	//initialize VGA controllers
	pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
	if(pixel_buf == NULL){
		printf("can't find pixel buffer device\n");
	}
	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 1);

	char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
	if(char_buf == NULL){
		printf("can't find char buffer device\n");
	}
	alt_up_char_buffer_clear(char_buf);

	memset(frequencyInfo, 0, sizeof(frequencyInfo));

	xNewConfigValueMutex = xSemaphoreCreateMutex();

	//Create queue for display
	xVGAQueue = xQueueCreate( 10, sizeof(VGAFrequencyInfo));

	//Create draw task
	xTaskCreate( PRVGADraw_Task, "DrawTsk", configMINIMAL_STACK_SIZE, NULL, 1, &PRVGADraw );

}

void VGA_nextConfigType(bool setValue) {
	xSemaphoreTake(xNewConfigValueMutex, portMAX_DELAY);
	if (setValue) {
		memcpy(configValues[configType], newConfigValue,KB_KEYBUFFER_SIZE);
	}
	newConfigValue[0] = '\0';
	xSemaphoreGive(xNewConfigValueMutex);
	
	if (configType >= 2) {
		configType = 0;
	}
	else {
		configType++;
	}
}
