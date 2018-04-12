/*
 * vga.h
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris
 */

#ifndef VGA_H_
#define VGA_H_

#include "FreeRTOS/queue.h"

#define VGA_PERIOD 16
#define VGA_CONFIG_TYPES_COUNT 3

typedef struct {
	bool stable;
	float freq;
    float derivative;
} VGAFrequencyInfo;

void VGA_start();

QueueHandle_t VGA_getQueueHandle();

void VGA_nextConfigType(bool setValue);

#endif /* VGA_H_ */
