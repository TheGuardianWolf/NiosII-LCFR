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

void Switch_start();

bool Switch_getState(uint8_t i);

#endif /* LOAD_SWITCH_H_ */
