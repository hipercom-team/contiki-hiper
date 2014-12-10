/*---------------------------------------------------------------------------
 *                      Basic clock sender
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

#include <stdio.h>

#include "contiki.h"
#include "string.h"
#include "net/rime.h"
#include "net/rime/unicast.h"
#include "random.h"

//#include "dev/adxl345.h"
#include "node-id.h"
#include "cc2420.h"

#include "../low-level.c"

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

unsigned char send_buffer[140];
unsigned char recv_buffer[140];

#define CONTENT_SIZE 100

/* 
   Basic frame
   0x41 0x88 <seq> <pad id = 0xcb 0xab> <dst = 0xffff> <src = 0x0500>
*/

#define SEND_DELAY CLOCK_SECOND

PROCESS(sender_thread, "sender");
PROCESS_THREAD(sender_thread, ev, data)
{
  static long int i = 0;
  static int originator;

  static struct etimer wait_timer;

  PROCESS_BEGIN();

  originator = node_id;
  memset(send_buffer, 0xa5, sizeof(send_buffer));
  memcpy(send_buffer+sizeof(i), &originator, sizeof(originator));

  for (;;) {

    unsigned long clock = clock_seconds();

    send_buffer[0] = 0x41;      /* FCS[1] */
    send_buffer[1] = 0x88;      /* FCS[2] */
    send_buffer[2] = i & 0xff;  /* seqno */
    send_buffer[3] = 0xcd;      /* PANID[1] */
    send_buffer[4] = 0xab;      /* PANID[2] */
    send_buffer[5] = 0xff;      /* dst[1] */
    send_buffer[6] = 0xff;      /* dst[2] */
    send_buffer[7] = node_id;   /* src[1] */
    send_buffer[8] = 0;         /* src[2] */
    send_buffer[9] = (clock>>24lu) & 0xffu;
    send_buffer[10] = (clock>>16lu) & 0xffu;
    send_buffer[11] = (clock>>8lu) & 0xffu;
    send_buffer[12] = (clock) & 0xffu;
    
    //uint16_t my_tar = TAR;
    //send_buffer[9] = my_tar >> 8;
    //send_buffer[10] = my_tar & 0xffu;

    uint8_t status = 0;
    do {
      CC2420_GET_STATUS(status);
    } while(status & BV(CC2420_TX_ACTIVE));

    int tx_status = cc2420_send(send_buffer, 13);
    if (tx_status == RADIO_TX_OK) {
      etimer_set(&wait_timer, SEND_DELAY);
      i++;
    } else {
      etimer_set(&wait_timer, 1);
    }
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
  }
  PROCESS_END();
}

#define CHANNEL 26

PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  cc2420_set_channel(CHANNEL);
  cc2420_set_send_with_cca(1);
  process_start(&sender_thread, NULL);

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
