/*
 * keyboard.h
 *
 *  Created on: 24/03/2018
 *      Author: chris
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "altera_up_avalon_ps2.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"

void ps2_isr(void* ps2_device, alt_u32 id);
void init_kb();


#endif /* KEYBOARD_H_ */
