/*
 * keyboard.h
 *
 *  Created on: 24/03/2018
 *      Author: chris
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdint.h>

#define KB_KEYBUFFER_SIZE 9
#define KB_TAB 0x0D
#define KB_BACKSPACE 0x66
#define KB_ENTER 0x5A

void KB_start();

uint8_t KB_getKeyBufferType();

void KB_getKeyBuffer(char* buf);

void KB_setKeyBuffer(char* buf);

void KB_setKey(size_t keyIndex, char k);


#endif /* KEYBOARD_H_ */
