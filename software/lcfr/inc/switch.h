/*
 * switches.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef SWITCH_H_
#define SWITCH_H_

#include <stdbool.h>
#include <stdint.h>

#define SWITCH_COUNT 5
#define SWITCH_PERIOD 50  // ms

#define EVENT_SWITCH_ON(x) 247 + x
#define EVENT_SWITCH_OFF(x) 242 + x

void Switch_start();

bool Switch_getState(uint8_t i);

#endif /* LOAD_SWITCH_H_ */
