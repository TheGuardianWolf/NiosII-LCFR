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

typedef void (*ButtonEventHandler)(void);

void Button_start();

void Button_ISRAttachHook(uint8_t i, ButtonEventHandler handler);

void Button_ISRDetatchHook(uint8_t i);

#endif /* BUTTON_H_ */
