
/**
 * \file
 *         Simple Hello World program
 * \author
 *         Cedric Adjih <Cedric.Adjih@inria.fr>
 */

#include <stdio.h>

#include "contiki.h"
#include "etimer.h"

/*---------------------------------------------------------------------------*/

PROCESS(example_process, "Example process");
AUTOSTART_PROCESSES(&example_process);

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_process, ev, data)
{
  static struct etimer et;
  static uint16_t count = 0;

  PROCESS_BEGIN();

  for (;;) {
    printf("Hello, world - %u\n", count);
    count++;

    etimer_set(&et, CLOCK_CONF_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
  
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
