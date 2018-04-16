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

void Switch_getState(bool * buf);

#endif /* LOAD_SWITCH_H_ */
