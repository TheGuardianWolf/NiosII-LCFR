/*
 * keyboard.c
 *
 *  Created on: Mar 25, 2018
 *      Author: chris
 */
#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "altera_up_avalon_ps2.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"

static unsigned char keyInput;

void ps2_isr(void* ps2_device, alt_u32 id){
	alt_up_ps2_read_data_byte_timeout(ps2_device, &keyInput);
	printf("Scan code: %x\n", keyInput);
}

void KB_start(){
	//enable interrupt for keyboard
	alt_up_ps2_dev * ps2_kb = alt_up_ps2_open_dev(PS2_NAME);
	alt_up_ps2_enable_read_interrupt(ps2_kb);
	alt_irq_register(PS2_IRQ, ps2_kb, ps2_isr);
}
