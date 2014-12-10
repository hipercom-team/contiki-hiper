/*---------------------------------------------------------------------------
 *                        Test for leds
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/


#include <stdio.h>
#include "contiki.h"

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

#define MY_WAIT(x) \
  etimer_set(&wait_timer, x); \
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

PROCESS_THREAD(init_process, ev, data)
{
  static struct etimer wait_timer;

  PROCESS_BEGIN();
  
  for (;;) {

    MY_LED_OFF(MY_R | MY_G | MY_B);
    MY_WAIT(CLOCK_SECOND);
    MY_LED_ON(MY_R);
    MY_WAIT(CLOCK_SECOND/2);
    MY_LED_TOGGLE(MY_B);
    MY_WAIT(CLOCK_SECOND/2);
    MY_LED_ON(MY_G);
    MY_WAIT(CLOCK_SECOND);
    MY_LED_OFF(MY_R);
    MY_WAIT(CLOCK_SECOND);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
