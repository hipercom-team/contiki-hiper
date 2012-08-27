/*---------------------------------------------------------------------------
 *                         Basic DAC output
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

#include "contiki.h"

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*--------------------*/

/*
  Basic DAC test: 
  - on the Zolertia Z1: (I think)
    . ground is at square pin on north port
    . digital-out is on the east port (pin 30, black cable on -idget connector)
    . analog out (DAC) is on north port, top, next to second [right]connector.
  - on the TelosB: (I think)
    . ground is at pin 9
    . digital-out (GIO1) is at 3 (next to 1) in the 6-pin expension part
    . analog out (DAC) is at 1 in the 6-pin expension part
 */

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

PROCESS_THREAD(init_process, ev, data)
{
  static struct etimer wait_timer;

  PROCESS_BEGIN();
  //watchdog_stop();
  my_timerb_init(0); /* set up timerb with 32 kHz source and int. counter */

  MY_LED_ON(MY_R);

  //my_dac_init();
  my_io_init();
  for (;;) {
    MY_LED_TOGGLE(MY_R | MY_B);
    //unsigned int value = my_get_clock() & 0xff;
    //if (value&1) value = (0xff-value);
    //my_dac_set_output(value << 4);
    my_io_toggle();
    etimer_set(&wait_timer, CLOCK_CONF_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
