/*
 * switches.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef SWITCHES_H_
#define SWITCHES_H_

#include <stdbool.h>
#include <stdint.h>

#define LOAD_SWITCH_MAX 5

void LoadSwitch_start();

bool LoadSwitch_getState(uint8_t i);


#endif /* SWITCHES_H_ */
