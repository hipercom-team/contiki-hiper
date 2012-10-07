/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A simple program for testing the adxl345 on-board accelerometer of the
 *         Zolertia Z1. Enables interrupts and registers callbacks for them. Then
 *         starts a constantly running readout of acceleration data.
 * \author
 *         Marcus Lundén, SICS <mlunden@sics.se>
 *         Enric M. Calvo, Zolertia <ecalvo@zolertia.com>
 */

#include <stdio.h>
#include "contiki.h"
#include "serial-shell.h"
#include "shell-ps.h"
#include "shell-file.h"
#include "shell-text.h"
#include "dev/adxl345.h"

#include "node-id.h"

#include "net/rime.h"


#define LED_INT_ONTIME        CLOCK_SECOND/2
#define LED_SLOW_INT_ONTIME        (CLOCK_SECOND)
#define ACCM_READ_INTERVAL    CLOCK_SECOND

static process_event_t ledOff_event;
static process_event_t ledOffSlow_event;
/*---------------------------------------------------------------------------*/
PROCESS(accel_process, "Test Accel process");
PROCESS(led_process, "LED handling process");
PROCESS(led_slow_process, "LED [slow] handling process");
AUTOSTART_PROCESSES(&accel_process, &led_process, &led_slow_process);
/*---------------------------------------------------------------------------*/
/* As several interrupts can be mapped to one interrupt pin, when interrupt 
    strikes, the adxl345 interrupt source register is read. This function prints
    out which interrupts occurred. Note that this will include all interrupts,
    even those mapped to 'the other' pin, and those that will always signal even if
    not enabled (such as watermark). */

void
print_int(uint16_t reg){
#define ANNOYING_ALWAYS_THERE_ANYWAY_OUTPUT 0
#if ANNOYING_ALWAYS_THERE_ANYWAY_OUTPUT
  if(reg & ADXL345_INT_OVERRUN) {
    printf("Overrun ");
  }
  if(reg & ADXL345_INT_WATERMARK) {
    printf("Watermark ");
  }
  if(reg & ADXL345_INT_DATAREADY) {
    printf("DataReady ");
  }
#endif
#ifdef WITH_DISPLAY
  if(reg & ADXL345_INT_FREEFALL) {
    printf("Freefall ");
  }
  if(reg & ADXL345_INT_INACTIVITY) {
    printf("InActivity ");
  }
  if(reg & ADXL345_INT_ACTIVITY) {
    printf("Activity ");
  }
  if(reg & ADXL345_INT_DOUBLETAP) {
    printf("DoubleTap ");
  }
  if(reg & ADXL345_INT_TAP) {
    printf("Tap ");
  }
  printf("\n");
#endif
}

/*---------------------------------------------------------------------------*/

static struct abc_conn abc_connection;

#define MAX_NODE 10
unsigned int seqnum_table[MAX_NODE];
unsigned int nodeid_table[MAX_NODE];

#define SEQNUM_NONE 0xffffu
#define NODEID_NONE 0xffffu

void reset_table()
{
  int i;
  for (i=0; i<MAX_NODE; i++)  {
    seqnum_table[i] = SEQNUM_NONE;
    nodeid_table[i] = NODEID_NONE;
  }
}

u8_t repeated_message[3];

static void general_packet_recv(struct abc_conn *c)
{ 
  int i;
  printf("recu\n");
  if(packetbuf_datalen() < 3) 
    return;
  
  //dump_data(packetbuf_dataptr(), packetbuf_datalen());
  u8_t* packet = ((unsigned char*)(packetbuf_dataptr()));

  u8_t sender_nodeid = packet[1];
  u8_t sender_seqnum = packet[2];

  if (sender_nodeid == node_id)
    return; /* someone repeated my own message */

  printf("recu: %d %d\n", sender_nodeid, sender_seqnum);
  
  int entry_index = -1;
  for (i=0; i<MAX_NODE; i++) {
    if (sender_nodeid == nodeid_table[i]) {
      if (sender_seqnum == seqnum_table[i]) 
	return; /* already received */
      else entry_index = i;
    } else if (entry_index == -1 && nodeid_table[i] == NODEID_NONE) {
      entry_index = i;
    }
  }
 
  if (entry_index == -1) {
    printf("error: seqnum table full!\n");
    return;
  }

  nodeid_table[entry_index] = sender_nodeid;
  seqnum_table[entry_index] = sender_seqnum;

  unsigned char state = packet[0];  
  L_OFF(LEDS_R + LEDS_G + LEDS_B);
  if (state == 0xc3)
    L_ON(LEDS_R);
  else if (state == 0xe3)
    L_ON(LEDS_G);
  else if (state == 0x87)
    L_ON(LEDS_B);

  process_post(&led_slow_process, ledOffSlow_event, NULL);

  memcpy(repeated_message, packet, 3);
}


const static struct abc_callbacks abc_callback = { general_packet_recv };

static unsigned int seq_num = 0;
void send_info(u8_t reg)
{
  u8_t packet[3];
  packet[0] = reg;
  packet[1] = node_id;
  packet[2] = seq_num++;
  packetbuf_copyfrom(packet,sizeof(packet));
  abc_send(&abc_connection);
}

/*---------------------------------------------------------------------------*/
/* accelerometer free fall detection callback */

void
accm_ff_cb(u8_t reg){
  L_ON(LEDS_B);
  process_post(&led_process, ledOff_event, NULL);
  printf("~~[%u] Freefall detected! (0x%02X) -- ", ((u16_t) clock_time())/128, reg);
  print_int(reg);
  send_info(reg);
}
/*---------------------------------------------------------------------------*/
/* accelerometer tap and double tap detection callback */

void
accm_tap_cb(u8_t reg){
  process_post(&led_process, ledOff_event, NULL);
  if(reg & ADXL345_INT_DOUBLETAP){
    L_ON(LEDS_G);
    printf("~~[%u] DoubleTap detected! (0x%02X) -- ", ((u16_t) clock_time())/128, reg);
  } else {
    L_ON(LEDS_R);
    printf("~~[%u] Tap detected! (0x%02X) -- ", ((u16_t) clock_time())/128, reg);
  }
  print_int(reg);
  send_info(reg);
}
/*---------------------------------------------------------------------------*/
/* When posted an ledOff event, the LEDs will switch off after LED_INT_ONTIME.
      static process_event_t ledOff_event;
      ledOff_event = process_alloc_event();
      process_post(&led_process, ledOff_event, NULL);
*/

static struct etimer ledETimer;
PROCESS_THREAD(led_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&ledETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ledETimer));
    L_OFF(LEDS_R + LEDS_G + LEDS_B);
  }
  PROCESS_END();
}

/* CA: for switching off leds when received */
static struct etimer ledETimerSlow;
PROCESS_THREAD(led_slow_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOffSlow_event);
    etimer_set(&ledETimerSlow, LED_SLOW_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ledETimerSlow));
    L_OFF(LEDS_R + LEDS_G + LEDS_B);

    packetbuf_copyfrom(repeated_message,sizeof(repeated_message));
    abc_send(&abc_connection);
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/*  Returns a string with the argument byte written in binary.
    Example usage:
      printf("Port1: %s\n", char2bin(P1IN));
*/    
/*
static u8_t b[9];

static u8_t
*char2bin(u8_t x) {
  u8_t z;
  b[8] = '\0';
  for (z = 0; z < 8; z++) {
    b[7-z] = (x & (1 << z)) ? '1' : '0';
  }
  return b;
}
*/


/*---------------------------------------------------------------------------*/
/* Main process, setups  */

static struct etimer et;

PROCESS_THREAD(accel_process, ev, data) {
  PROCESS_BEGIN();
  {
    int16_t x, y, z;

    reset_table();
    cc2420_set_channel(12);
    abc_open(&abc_connection, 128, &abc_callback);

    serial_shell_init();
    shell_ps_init();
    shell_file_init();  // for printing out files
    shell_text_init();  // for binprint

    /* Register the event used for lighting up an LED when interrupt strikes. */
    ledOff_event = process_alloc_event();
    ledOffSlow_event = process_alloc_event();

    /* Start and setup the accelerometer with default values, eg no interrupts enabled. */
    accm_init();

    /* Register the callback functions for each interrupt */
    ACCM_REGISTER_INT1_CB(accm_ff_cb);
    ACCM_REGISTER_INT2_CB(accm_tap_cb);

    /* Set what strikes the corresponding interrupts. Several interrupts per pin is 
      possible. For the eight possible interrupts, see adxl345.h and adxl345 datasheet. */
    accm_set_irq(ADXL345_INT_FREEFALL, ADXL345_INT_TAP + ADXL345_INT_DOUBLETAP);

    while (1) {
	    x = accm_read_axis(X_AXIS);
	    y = accm_read_axis(Y_AXIS);
	    z = accm_read_axis(Z_AXIS);
#ifdef WITH_DISPLAY
	    printf("x: %d y: %d z: %d\n", x, y, z);
#endif
      etimer_set(&et, ACCM_READ_INTERVAL);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

