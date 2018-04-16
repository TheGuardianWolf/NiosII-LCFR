/*
 * event.c
 *
 *  Created on: Apr 16, 2018
 *      Author: Pat
 */

#include <limits.h>
#include "system.h"
#include "event.h"
#include "sys/alt_irq.h"
#include "sys/alt_timestamp.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_timer_regs.h"
#include <stdio.h>

static uint32_t overflowCount = 0;

static void Event_timestampISR(void* base, alt_u32 id) {
//	alt_irq_context c;
//	c = alt_irq_disable_all();
	IOWR_ALTERA_AVALON_TIMER_STATUS (base, 0);
//	IORD_ALTERA_AVALON_TIMER_CONTROL (base);
//	alt_timestamp_start();
	overflowCount++;
}

void Event_start () {
  /* set to free running mode */
	IOWR_ALTERA_AVALON_TIMER_CONTROL (TIMER1US_BASE,ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);



	  IOWR_ALTERA_AVALON_TIMER_PERIODL (TIMER1US_BASE, 0xFFFF);
	  IOWR_ALTERA_AVALON_TIMER_PERIODH (TIMER1US_BASE, 0xFFFF);
	  IOWR_ALTERA_AVALON_TIMER_CONTROL (TIMER1US_BASE,
					ALTERA_AVALON_TIMER_CONTROL_ITO_MSK  |
					ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
					ALTERA_AVALON_TIMER_CONTROL_START_MSK);


  alt_irq_register(TIMER1US_IRQ, TIMER1US_BASE, Event_timestampISR);
}

uint64_t Event_getTimestamp() {
    IOWR_ALTERA_AVALON_TIMER_SNAPL (TIMER1US_BASE, 0);
    alt_timestamp_type lower = IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER1US_BASE) & ALTERA_AVALON_TIMER_SNAPL_MSK;
    alt_timestamp_type upper = IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER1US_BASE) & ALTERA_AVALON_TIMER_SNAPH_MSK;

    uint32_t timerValue =  (0xFFFFFFFF - ((upper << 16) | lower));

    return (uint64_t)overflowCount * (4294967295UL) + (uint64_t)timerValue;
}

