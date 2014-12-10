/*---------------------------------------------------------------------------
 *               Test for cycle counter using TimerB
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include "contiki.h"

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*--------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  my_timerb_init(1); /* set up timerb with 8Mhz source and int. counter */
  //my_timerb_init(0); /* set up timerb with 32Khz source and int. counter */

  for (;;) {
    uint32_t t1 = my_get_clock();
    uint32_t t2 = my_get_clock();
    uint32_t t3 = my_get_clock();
    uint32_t t4 = my_get_clock();
    uint32_t t5 = my_get_clock();

    printf("t2-t1=%lu t3-t2=%lu t4-t3=%lu t5-t4=%lu [cycles]\n",
	   t2-t1, t3-t2, t4-t3, t5-t4);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
