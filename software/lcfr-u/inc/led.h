/*
 * switches.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef LED_H_
#define LED_H_

#include <stdbool.h>
#include <stdint.h>

#define LED_COUNT 5
#define LED_PERIOD 50  // ms

void LED_start();

bool LED_getState(uint8_t i);


#endif /* LED_H_ */
