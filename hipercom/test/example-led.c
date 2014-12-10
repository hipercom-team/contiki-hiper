/*---------------------------------------------------------------------------
 *                        Test for leds
 * Author: Cedric Adjih (Inria)
 *---------------------------------------------------------------------------
 * Copyright (c) 2012-2014, Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
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
