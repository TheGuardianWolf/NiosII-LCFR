/*
 * vga.h
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris
 */

#ifndef VGA_H_
#define VGA_H_

#include <stdbool.h>
#include "FreeRTOS/queue.h"

#define VGA_PERIOD 33
#define VGA_CONFIG_TYPES_COUNT 3

#define FREQPLT_ORI_X 101		//x axis pixel position at the plot origin
#define FREQPLT_GRID_SIZE_X 5	//pixel separation in the x axis between two data points
#define FREQPLT_ORI_Y 199.0		//y axis pixel position at the plot origin
#define FREQPLT_FREQ_RES 20.0	//number of pixels per Hz (y axis scale)

#define ROCPLT_ORI_X 101
#define ROCPLT_GRID_SIZE_X 5
#define ROCPLT_ORI_Y 259.0
#define ROCPLT_ROC_RES 0.5		//number of pixels per Hz/s (y axis scale)

#define MIN_FREQ 45.0 //minimum frequency to draw

typedef struct {
	bool stable;
	float instant;
    float derivative;
} VGAFrequencyInfo;

typedef struct{
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
} VGALine;

void VGA_start();

QueueHandle_t VGA_getQueueHandle();

void VGA_nextConfigType(bool setValue);

#endif /* VGA_H_ */
