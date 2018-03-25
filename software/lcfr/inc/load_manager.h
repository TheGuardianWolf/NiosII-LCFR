/*
 * managed_relay.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef LOAD_MANAGER_H_
#define LOAD_MANAGER_H_

#include <stdbool.h>

#define LOAD_MANAGER_PERIOD 25
#define LOAD_MANAGER_LOADS 5
#define LOAD_MANAGER_GRACE 500

typedef enum {
	DISABLED,
	SHED,
	ENABLED
} ManagedState;

void LoadManager_start();

bool LoadManager_getActive();

void LoadManager_toggleActive();

ManagedState LoadManager_getState(uint8_t i);

#endif /* LOAD_MANAGER_H_ */
