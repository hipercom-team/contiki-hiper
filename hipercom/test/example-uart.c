/*---------------------------------------------------------------------------
 *                      Test for UART functions
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

  watchdog_stop();
  my_timerb_init(0); /* set up timerb with 32 kHz source and int. counter */

  my_uart_init();
  char data = '?';
  for (;;) {
    my_uart_write('*');
    while (!my_uart_has_data()) ;
    data = my_uart_read();
    my_uart_write(data+1);
  } 

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
