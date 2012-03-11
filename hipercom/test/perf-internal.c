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

unsigned long get_current_time()
{
  unsigned long t1, t2;
  unsigned long count;
  do {
    t1 = clock_seconds();
    count = clock_time();
    t2 = clock_seconds();
  } while(t1 != t2);

  return (t1 * CLOCK_CONF_SECOND) + (count % CLOCK_CONF_SECOND);
}

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

unsigned char send_buffer[140];
unsigned char recv_buffer[140];

#define CONTENT_SIZE 100
//#define CONTENT_SIZE 4

static volatile long last_seq_num = -1;
static volatile long stat_bad_size = 0;
static volatile long stat_bad_originator = 0;
static volatile long stat_lost = 0;
static volatile long stat_recv = 0;
static volatile unsigned long first_packet_time = 0;
static volatile int last_originator = -1;

extern volatile uint8_t cc2420_sfd_counter;

static void general_packet_recv(struct abc_conn *c)
{
  long int seq_num = -1;
  int originator;
  if (packetbuf_datalen() != CONTENT_SIZE)  {
    stat_bad_size++;
    return;  
  }

  stat_recv ++;
  memcpy(&seq_num, packetbuf_dataptr(),  sizeof(seq_num));
  memcpy(&originator, packetbuf_dataptr()+sizeof(seq_num),  
	 sizeof(originator));
  if (last_seq_num != -1) {
    if (originator != last_originator) {
      stat_bad_originator++;
      return;
    }
    stat_lost += (seq_num - 1) - last_seq_num;
  } else {
    last_originator = originator;
    first_packet_time = get_current_time();
  }
  last_seq_num = seq_num;
}

/*---------------------------------------------------------------------------*/

static struct abc_conn abc_connection;
const static struct abc_callbacks abc_callback = { general_packet_recv };

/*---------------------------------------------------------------------------*/

#if !defined(ACTION_SEND_FAST) && !defined(SEND_DELAY)
#define SEND_DELAY (CLOCK_SECOND/2)
#endif /* ACTION_SEND_FAST */

PROCESS(sender_thread, "sender");

/* 
   Basic frame
   0x41 0x88 <seq> <pad id = 0xcb 0xab> <dst = 0xffff> <src = 0x0500>
*/

#define DIRECT_SEND

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

#ifndef DIRECT_SEND
    memcpy(send_buffer, &i, sizeof(i));
    i++;
    packetbuf_copyfrom(send_buffer, CONTENT_SIZE);
    abc_send(&abc_connection);
#else
    send_buffer[0] = 0x41;
    send_buffer[1] = 0x88;
    send_buffer[2] = i & 0xff;
    send_buffer[3] = 0xcd;
    send_buffer[4] = 0xab;
    send_buffer[5] = 0xff;
    send_buffer[6] = 0xff;
    send_buffer[7] = node_id;
    send_buffer[8] = 0;
    uint8_t status = 0;
    do {
      CC2420_GET_STATUS(status);
    } while(status & BV(CC2420_TX_ACTIVE));

    cc2420_send(send_buffer, 9);
#endif

#ifdef SEND_DELAY
  etimer_set(&wait_timer, SEND_DELAY);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
#endif
  }
  PROCESS_END();
}

/*--------------------------------------------------*/

PROCESS(stat_thread, "stat");

PROCESS_THREAD(stat_thread, ev, data)
{
  static struct etimer wait_timer;

  PROCESS_BEGIN();

  for (;;) {
    etimer_set(&wait_timer, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    
    unsigned long interval = get_current_time() - first_packet_time;
    printf("received %ld/s nb=%ld lost=%ld bad_orig=%ld last_seq_num=%ld\n", 
	   (stat_recv * CLOCK_SECOND)/ interval, 
	   stat_recv, stat_lost, stat_bad_originator, last_seq_num);

    last_seq_num = -1;
    stat_lost = 0;
    stat_recv = 0;
    first_packet_time = 0;
    last_originator = -1;
    stat_bad_originator = 0;
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  //serial_shell_init();
  abc_open(&abc_connection, 0xee, &abc_callback);

  cc2420_set_channel(CHANNEL);
  cc2420_set_send_with_cca(0);

#if defined(ACTION_SEND) || defined(ACTION_SEND_FAST)
  process_start(&sender_thread, NULL);
#endif

#ifdef ACTION_RECV
  process_start(&stat_thread, NULL);  
#endif

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
