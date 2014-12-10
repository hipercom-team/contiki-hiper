/*---------------------------------------------------------------------------
 *                         Basic DAC output
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
  PROCESS_BEGIN();

  watchdog_stop();
  my_timerb_init(0); /* set up timerb with 32 kHz source and int. counter */

  MY_LED_ON(MY_R);

  my_dac_init();
  my_io_init();
  for (;;) {
    MY_LED_TOGGLE(MY_R | MY_B);
    unsigned int value = my_get_clock() & 0xff;
    if (value&1) value = (0xff-value);
    my_dac_set_output(value << 4);
    my_io_toggle();
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
