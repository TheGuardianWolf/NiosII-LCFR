/*
 * switches.h
 *
 *  Created on: 23/03/2018
 *      Author: lichk
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_COUNT 1

#define EVENT_BUTTON_PRESSED 252

// typedef void (*ButtonEventHandler)(void);

void Button_start();

#endif /* BUTTON_H_ */
