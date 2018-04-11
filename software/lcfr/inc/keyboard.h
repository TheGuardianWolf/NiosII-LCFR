/*
 * keyboard.h
 *
 *  Created on: 24/03/2018
 *      Author: chris
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define KB_KEYBUFFER_SIZE 16

void KB_start();

void KB_getKeyBuffer(char* buf);

void KB_setKeyBuffer(char* buf);

void KB_setKey(size_t keyIndex, char k);


#endif /* KEYBOARD_H_ */
