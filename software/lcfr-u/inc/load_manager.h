/*
 * managed_relay.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef LOAD_MANAGER_H_
#define LOAD_MANAGER_H_

#include <stdbool.h>

#include "freertos/queue.h"
#include "system.h"

#define LOAD_MANAGER_LOADS 5
#define LOAD_MANAGER_GRACE 500

typedef enum {
	DISABLED,
	ENABLED,
	SHED
} ManagedState;

typedef struct { 
    uint64_t min;
    uint64_t max;
    uint64_t average;
    uint32_t averageSamples; 
} ReactionTimes; 

void LoadManager_start();

QueueHandle_t LoadManager_getQueueHandle();

ManagedState LoadManager_getState(uint8_t i);

ReactionTimes LoadManager_getReactionTimes();

#endif /* LOAD_MANAGER_H_ */
