#include <stdio.h>
#include <stdlib.h>
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "display.h"


#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"

#include "VGA.h"
#include "keyboard.h"
#include "config.h"

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

static alt_up_pixel_buffer_dma_dev *pixel_buf;
static alt_up_char_buffer_dev *char_buf;
static const TickType_t xFrequency = VGA_PERIOD * portTICK_PERIOD_MS;
static enum config_type current_type = lower_freq;

/****** VGA display ******/

void PRVGADraw_Task(void *pvParameters ){
	printf("enter vga");
	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);
	alt_up_char_buffer_clear(char_buf);

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

	alt_up_char_buffer_string(char_buf, "System Status: ", 4, 46);
	alt_up_char_buffer_string(char_buf, "Lower Frequency Threshold: ", 4, 48);
	alt_up_char_buffer_string(char_buf, "Change in Frequency Threshold: ", 4, 50);
	alt_up_char_buffer_string(char_buf, "Enter New Lower Frequency Threshold: ", 38, 48);
	alt_up_char_buffer_string(char_buf, "Enter Change in Frequency Threshold: ", 40, 50);

	float freq[100], dfreq[100];
	//struct display_info display_array[100];
	int i = 99, j = 0;
	Line line_freq, line_roc;
	unsigned char key;
	unsigned char* current_key_array_lower = "45";
	unsigned char* current_key_array_change = "10";
	unsigned char key_array_lower[16];
	unsigned char key_array_change[16];
	unsigned int lower_i = 0;
	unsigned int change_i = 0;

	struct display_info display;
	TickType_t xLastWakeTime;

	while(1){
		xLastWakeTime = xTaskGetTickCount();

		if(xSemaphoreTake(KB_getQueueHandle(), portMAX_DELAY) == pdTRUE) {
			key = getKey();
			xSemaphoreGive(getKeySemaphore());
		}

		//receive frequency data from queue
		if (xQueueReceive(xVGAQueue, &display, portMAX_DELAY) == pdTRUE) {

			if (display.stable == true) {
				alt_up_char_buffer_string(char_buf, "Stable  ", 19, 46);
			}
			else {
				alt_up_char_buffer_string(char_buf, "Unstable", 19, 46);
			}

			freq[i] = display.freq;

			//calculate frequency RoC

			if(i==0){
				dfreq[0] = (freq[0]-freq[99]) * 2.0 * freq[0] * freq[99] / (freq[0]+freq[99]);
			}
			else{
				dfreq[i] = (freq[i]-freq[i-1]) * 2.0 * freq[i]* freq[i-1] / (freq[i]+freq[i-1]);
			}

			if (dfreq[i] > 100.0){
				dfreq[i] = 100.0;
			}


			i =	++i%100; //point to the next data (oldest) to be overwritten

		}

		if(key >= '0' && key <= '9') {
			if(current_type == lower_freq) {
				key_array_lower[lower_i] = key;
				lower_i++;
			}
			else {
				key_array_change[change_i] = key;
				change_i++;
			}
		}
		else if(key == 10) {
			int j;
			current_key_array_lower = &key_array_lower;
			if (current_type == lower_freq) {
				for (j = 0; j < 16; j++) {
					key_array_lower[j] = '';
				}
				lower_i = 0;
			}
			else {
				for (j = 0; j < 16; j++) {
					key_array_change[j] = '';
				}
				change_i = 0;
			}
		}
		else {
			change_type();
		}

		alt_up_char_buffer_string(char_buf, current_key_array_lower, 32, 48);
		alt_up_char_buffer_string(char_buf, current_key_array_change, 35, 50);
		alt_up_char_buffer_string(char_buf, &key_array_lower, 77, 48);
		alt_up_char_buffer_string(char_buf, &key_array_change, 77, 50);

		//clear old graph to draw new graph
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 0, 639, 199, 0, 0);
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 201, 639, 299, 0, 0);

		for(j=0;j<99;++j){ //i here points to the oldest data, j loops through all the data to be drawn on VGA
			if (((int)(freq[(i+j)%100]) > MIN_FREQ) && ((int)(freq[(i+j+1)%100]) > MIN_FREQ)){
				//Calculate coordinates of the two data points to draw a line in between
				//Frequency plot
				line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
				line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j)%100] - MIN_FREQ));

				line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
				line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j+1)%100] - MIN_FREQ));

				//Frequency RoC plot
				line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
				line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j)%100]);

				line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
				line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j+1)%100]);

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

void change_type() {
	if (current_type == lower_freq) {
		current_type = change_in_freq;
	}
	else {
		current_type = lower_freq;
	}
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

	//Create draw task
	xTaskCreate( PRVGADraw_Task, "DrawTsk", configMINIMAL_STACK_SIZE, NULL, 2, &PRVGADraw );

	//Create queue for display
	xVGAQueue = xQueueCreate( 100, sizeof(struct display_info));
}
