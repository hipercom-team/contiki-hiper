/*---------------------------------------------------------------------------
 *                          PIR Detector
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

#include <stdio.h>

#include "contiki.h"
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

static void pir_packet_recv(struct abc_conn *c);
static void accel_packet_recv(struct abc_conn *c);

static struct abc_conn abc_connection;
const static struct abc_callbacks abc_callback = { pir_packet_recv };

static struct abc_conn abc_accel_connection;
const static struct abc_callbacks abc_accel_callback = { accel_packet_recv };

/*--------------------------------------------------*/
#define PIR_PACKET_SIZE (2+sizeof(seq_num))
static unsigned int seq_num = 0;

static void pir_packet_send(uint8_t status)
{
  u8_t packet[PIR_PACKET_SIZE];
  packet[0] = node_id;
  packet[1] = status;
  memcpy(packet+2, &seq_num, sizeof(seq_num));
  seq_num++;

  packetbuf_copyfrom(packet,sizeof(packet));
  abc_send(&abc_connection);
}

static unsigned int recv_seq_num = 0;
static int recv_pir_status = -1;
static int recv_node_id = -1;

static void pir_packet_recv(struct abc_conn *c)
{ 
  u8_t packet[PIR_PACKET_SIZE];

  if(packetbuf_datalen() < PIR_PACKET_SIZE) 
    return;

  memcpy(packet, packetbuf_dataptr(), PIR_PACKET_SIZE);
  recv_node_id = packet[0];
  recv_pir_status = packet[1];
  memcpy(&recv_seq_num, packet+2, sizeof(recv_seq_num));
}

/*---------------------------------------------------------------------------*/

static uint8_t accel_state = 0;
static int     accel_node_id = -1;
static int     accel_counter = 0;

#define ACCEL_PACKET_SIZE 3

static void accel_packet_recv(struct abc_conn *c)
{
  u8_t packet[ACCEL_PACKET_SIZE];

  if(packetbuf_datalen() < ACCEL_PACKET_SIZE) 
    return;

  memcpy(packet, packetbuf_dataptr(), ACCEL_PACKET_SIZE);
  accel_state = packet[0]; 
  accel_node_id = packet[1];
  accel_counter ++;
#if 0
  L_OFF(LEDS_R + LEDS_G + LEDS_B);
  if (state == 0xc3)
    L_ON(LEDS_R);
  else if (state == 0xe3)
    L_ON(LEDS_G);
  else if (state == 0x87)
    L_ON(LEDS_B);
#endif
}

/*---------------------------------------------------------------------------*/

int get_pir_status() 
{ return (P4IN & (1<<2)) == 0; }

#define MAX_INFO_DELAY (CLOCK_SECOND/2)

PROCESS_THREAD(init_process, ev, data)
{
  static struct etimer wait_timer;

  PROCESS_BEGIN();
  printf("Booting application PIR detector\n");

  // 0x77
  abc_open(&abc_connection, 0x77, &abc_callback); // start receiving broadcast
  abc_open(&abc_accel_connection, 128, &abc_accel_callback); // accel data

  L_OFF(LEDS_R + LEDS_G + LEDS_B);

  /* one second delay */
  etimer_set(&wait_timer, CLOCK_CONF_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

  cc2420_set_channel(12);

  /* Set up P4 output */
  P4SEL &= ~(1<<2); // P4.2    is selected as I/O
  P4DIR &= ~(1<<2); // P4.2    is input
  //P4OUT |= (1<<2); // P4.2    set to 1

  static clock_time_t last_clock = 0;
  static int last_pir_status = -1; /* no status was read */
  static int last_recv_pir_status = -1; /* no status was read */
  static unsigned int last_accel_counter = 0xffffu; /* no accel info was recv */
  for (;;) {
    int new_pir_status = get_pir_status();
    int new_recv_pir_status = recv_pir_status;
    
    if (new_pir_status != last_pir_status
	|| new_recv_pir_status != last_recv_pir_status
	|| last_accel_counter != accel_counter
	||  (clock_time() - last_clock > MAX_INFO_DELAY)) {
      printf("PIR %d %d %u %d %d %d %d\n",
	     new_pir_status, new_recv_pir_status, 
	     recv_seq_num, recv_node_id, 
	     (int)accel_counter, (int)accel_state, (int)accel_node_id);
      pir_packet_send(new_pir_status);
      last_pir_status = new_pir_status;
      last_recv_pir_status = new_recv_pir_status;
      last_accel_counter = accel_counter;
      last_clock = clock_time();

      if (new_pir_status == 1) { L_ON(LEDS_R); }
      else { L_OFF(LEDS_R); }

      if (new_recv_pir_status == 1) { L_ON(LEDS_B); }
      else { L_OFF(LEDS_B); }
    }

    if ( (last_clock & 64) != 0) { L_ON(LEDS_G); }
    else { L_OFF(LEDS_G); }
    
    etimer_set(&wait_timer, 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
