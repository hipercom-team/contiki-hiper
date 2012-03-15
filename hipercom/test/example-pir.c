/*---------------------------------------------------------------------------
 *                      Basic sender / receiver
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
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
