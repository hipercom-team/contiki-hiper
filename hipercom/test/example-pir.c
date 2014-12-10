/*---------------------------------------------------------------------------
 * Basic sender / receiver
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
//#include "shell.h"
//#include "serial-shell.h"
#include "clock.h"

#include "string.h"
#include "net/rime.h"
#include "net/rime/unicast.h"
#include "random.h"

#include "dev/adxl345.h"
#include "node-id.h"

#include "cc2420.h"

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_process, ev, data)
{
  static struct etimer wait_timer;

  PROCESS_BEGIN();

  etimer_set(&wait_timer, CLOCK_CONF_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

  /* Set up P4 output */
  P4SEL &= ~(1<<2); // P4.2    is selected as I/O
  P4DIR &= ~(1<<2); // P4.2    is input
  //P4OUT |= (1<<2); // P4.2    set to 1

  static int counter = 0;
  for (;;) {
    if (counter%64 == 0)
      printf("\n");

    printf("%d", P4IN & (1<<2));
    etimer_set(&wait_timer, CLOCK_CONF_SECOND/4);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    counter++;
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
