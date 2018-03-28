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

#define EVENT_LOAD_MANAGER_GRACE_EXPIRED 255

typedef enum {
	DISABLED,
	ENABLED,
	SHED
} ManagedState;

void LoadManager_start();

QueueHandle_t LoadManager_getQueueHandle();

ManagedState LoadManager_getState(uint8_t i);

#endif /* LOAD_MANAGER_H_ */
