/*
 * user_interface.h
 *
 *  Created on: 24/03/2018
 *      Author: chris
 */

#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_


#include <stdint.h>

void ps2_isr(void* ps2_device, uint32_t id);

void UserInterface_start();

void VGA_start();

void KB_start();




#endif /* USER_INTERFACE_H_ */
