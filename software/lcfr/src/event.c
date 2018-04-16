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
#include "altera_avalon_timer.h"
#include "altera_avalon_timer_regs.h"

static uint32_t overflowCount = 0;

static void Event_timestampISR(void* base, alt_u32 id) {
	overflowCount++;
}

void Event_start () {
  /* set to free running mode */
  IOWR_ALTERA_AVALON_TIMER_CONTROL (TIMER1US_BASE,
            ALTERA_AVALON_TIMER_CONTROL_ITO_MSK  |
            ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
            ALTERA_AVALON_TIMER_CONTROL_START_MSK);


  alt_irq_register (TIMER1US_IRQ, TIMER1US_BASE, Event_timestampISR);
}

uint64_t Event_getTimestamp() {
    IOWR_ALTERA_AVALON_TIMER_SNAPL (TIMER1US_BASE, 0);
    alt_timestamp_type lower = IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER1US_BASE) & ALTERA_AVALON_TIMER_SNAPL_MSK;
    alt_timestamp_type upper = IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER1US_BASE) & ALTERA_AVALON_TIMER_SNAPH_MSK;

    uint32_t timerValue =  (0xFFFFFFFF - ((upper << 16) | lower));

    return (uint64_t)overflowCount * UINT_MAX + (uint64_t)timerValue;
}

