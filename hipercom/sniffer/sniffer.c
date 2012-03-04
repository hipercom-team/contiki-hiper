/*---------------------------------------------------------------------------
 *                           Packet Sniffer
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

#include "contiki.h"
#include "dev/leds.h"
#include <stdio.h>

#include <signal.h>
#include <spi.h>

#include <io.h>


#include "net/netstack.h"

#include "net/rime.h"

#include "dev/cc2420.h"
#include "dev/cc2420_const.h"
#include "dev/spi.h"

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

#define SNIFFER_VERSION_MAJOR 0
#define SNIFFER_VERSION_MINOR 1


#define COMMAND_INVOKE_CODE1 'C'
#define COMMAND_INVOKE_CODE2 'I'

#define COMMAND_ANSWER_CODE1 'C'
#define COMMAND_ANSWER_CODE2 'A'

#define COMMAND_BUFFER_SIZE 256
static uint8_t command_buffer[COMMAND_BUFFER_SIZE];
static uint8_t command_length = 0;
static uint8_t command_index = 0;
typedef enum {
  StateNone,
  StateHeaderOne,
  StateHeaderLen,
  StateContent,
  StateComplete
} serial_command_state_t;

serial_command_state_t serial_command_state = StateNone;

uint8_t process_one_serial()
{
  uint8_t data = my_uart0_read();
  switch (serial_command_state) {
  case StateNone:
    if (data == COMMAND_INVOKE_CODE1)
      serial_command_state = StateHeaderOne;
    return 0;
    
  case StateHeaderOne:
    if (data == COMMAND_INVOKE_CODE2)
      serial_command_state = StateHeaderLen;
    else if (data != COMMAND_INVOKE_CODE1)
      serial_command_state = StateNone;
    else serial_command_state = StateHeaderOne; /* [for clarity] */
    return 0;

  case StateHeaderLen:
    if (data <= sizeof(command_buffer)) {
      command_length = data;
      command_index = 0;
      if (command_length > 0)
	serial_command_state = StateContent;
      else serial_command_state = StateComplete;
    } else {
      serial_command_state = StateNone;
    }
    return serial_command_state == StateComplete;

  case StateContent:
    command_buffer[command_index] = data;
    command_index ++;
    if (command_index == command_length)
      serial_command_state = StateComplete;
    return serial_command_state == StateComplete;

  case StateComplete:
    if (data == COMMAND_INVOKE_CODE1)
      serial_command_state = StateHeaderOne;
    else serial_command_state = StateNone;
    return 0;

  default:
    serial_command_state = StateNone;
    return 0;
  }
}

static inline
uint8_t has_serial_command(void)
{
  if (serial_command_state == StateComplete)
    return 1;
  else if (!my_uart0_has_data())
    return 0;
  else return process_one_serial();
}

/*---------------------------------------------------------------------------*/

#define OUTPUT_BUFFER_LOG2_SIZE 11 /* 2048 bytes */

#define OUTPUT_BUFFER_OFFSET_MASK ((1<<OUTPUT_BUFFER_LOG2_SIZE)-1)

#define OUTPUT_BUFFER_RESERVE 256  /* for inserting command reply(s) */

uint8_t output_buffer[(1<<OUTPUT_BUFFER_LOG2_SIZE)];
unsigned int producer_offset = 0;
unsigned int consumer_offset = 0;

#define _OUTPUT_BUFFER_HAS_FREE_SPACE_FOR(number_of_bytes)	\
  ((uint16_t)(consumer_offset-1 - producer_offset) > (number_of_bytes))

#define OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(number_of_bytes)	\
  _OUTPUT_BUFFER_HAS_FREE_SPACE_FOR((number_of_bytes) + OUTPUT_BUFFER_RESERVE)

static inline void append_output_buffer(uint8_t data)
{
  output_buffer[producer_offset] = data;
  producer_offset = (producer_offset + 1) & OUTPUT_BUFFER_OFFSET_MASK;
}

static inline void send_one_byte_buffer_to_serial(void)
{
  my_uart0_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
  consumer_offset = (consumer_offset+1) & OUTPUT_BUFFER_OFFSET_MASK;     
}

static void update_output_buffer(uint8_t should_reset, void* data, int length)
{
  if (should_reset) {
    producer_offset = 0;
    consumer_offset = 0;
  }
  if (data != NULL && length+3 < sizeof(output_buffer))  {
    append_output_buffer(COMMAND_ANSWER_CODE1);
    append_output_buffer(COMMAND_ANSWER_CODE2);
    append_output_buffer(length);
    int i;
    for (i=0;i<length;i++)
      append_output_buffer(((uint8_t*)data)[i]);
  }
}

static void run_serial_loop(void)
{
  for (;;) {
    while (!has_serial_command() && producer_offset != consumer_offset) {
      my_uart0_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
      consumer_offset = (consumer_offset+1) & OUTPUT_BUFFER_OFFSET_MASK;
    }

    if (has_serial_command())
      return;
  }
}

/*---------------------------------------------------------------------------*/

extern volatile uint8_t cc2420_packet_counter; // added in cc2420.c
uint8_t last_packet_counter = 0;
unsigned int lost_packet = 0;
uint32_t global_packet_counter = 0;

/* see cc2420_process */
#define COMMAND_ANSWER_HEADER_LEN \
  (7+sizeof(global_packet_counter)+sizeof(my_time_t))
#define RECEIVE_BUFFER_SIZE (PACKETBUF_SIZE+COMMAND_ANSWER_HEADER_LEN)
uint8_t receive_packet_buffer[RECEIVE_BUFFER_SIZE];
extern volatile uint16_t cc2420_sfd_start_time;
void my_process_packet(void)
{
  my_time_t timestamp = my_get_clock();

  int hdrlen = COMMAND_ANSWER_HEADER_LEN;
  int len = cc2420_driver.read(receive_packet_buffer+hdrlen, PACKETBUF_SIZE);
  global_packet_counter++;
  receive_packet_buffer[0] = COMMAND_ANSWER_CODE1;
  receive_packet_buffer[1] = COMMAND_ANSWER_CODE2;
  receive_packet_buffer[2] = len+hdrlen-3;

  receive_packet_buffer[3] = 'p';
  receive_packet_buffer[4] = lost_packet & 0xff;

  receive_packet_buffer[5] = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  receive_packet_buffer[6] = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

  memcpy(receive_packet_buffer+7, &global_packet_counter, 
	 sizeof(global_packet_counter));

  memcpy(receive_packet_buffer+7+sizeof(global_packet_counter), 
	 &timestamp, sizeof(timestamp));

  len += hdrlen;

  if (!OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(len)) {
    lost_packet++;
    leds_toggle(LEDS_GREEN);
  } else {
    int i;
    for (i=0; i<len; i++) {
      output_buffer[producer_offset] = receive_packet_buffer[i];
      producer_offset = (producer_offset + 1) & OUTPUT_BUFFER_OFFSET_MASK;
    }
  }
}

/*--------------------------------------------------*/

static inline
uint8_t has_packet(void)
{ return cc2420_packet_counter != last_packet_counter; } 


static void run_packet_loop(void)
{
  leds_on(LEDS_RED);
  for (;;) {
    if (has_packet()) {
      last_packet_counter = cc2420_packet_counter;
      leds_toggle(LEDS_BLUE);
      my_process_packet();
    }

    while (!has_packet() && !has_serial_command()
	   && producer_offset != consumer_offset) {
      my_uart0_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
      consumer_offset = (consumer_offset+1) & OUTPUT_BUFFER_OFFSET_MASK;
    }

    if (has_serial_command()) {
      leds_off(LEDS_RED);
      leds_off(LEDS_BLUE);
      return;
    }
  }
}

/*--------------------------------------------------*/

typedef struct {
  uint8_t  command_code1;
  uint8_t  command_code2;
  uint8_t  command_len;
  uint8_t  command_subcode;
  uint32_t timestamp;
  uint8_t  rssi_value;
} command_answer_rssi_t;

static void run_rssi_loop(void)
{
  leds_on(LEDS_GREEN);
  command_answer_rssi_t answer;
  memset(&answer, 0xee, sizeof(answer));
  answer.command_code1 = COMMAND_ANSWER_CODE1;
  answer.command_code2 = COMMAND_ANSWER_CODE2;
  answer.command_len = sizeof(command_answer_rssi_t) - 3;
  answer.command_subcode = 'r';

  cc2420_set_auto_flushrx(1); /* disable receiving packets by flushing them */

  for (;;) {

    /* append RSSI state is buffer is not full */
    if (OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(sizeof(answer))) {
      answer.rssi_value = (cc2420_rssi() + 55);
      answer.timestamp = my_get_clock();

      uint8_t i;
      for (i=0; i<sizeof(answer); i++)
	append_output_buffer( ((uint8_t*)&answer)[i] );
    }

    /* write output one byte on serial port if it is not busy */
    if (!MY_IS_UART0_BUSY && producer_offset != consumer_offset)
      send_one_byte_buffer_to_serial();

    /* if output buffer is almost full, write at least a block of bytes  */
    while (!OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(sizeof(answer))
	   && !has_serial_command())
      send_one_byte_buffer_to_serial();

    if (has_serial_command()) {
      cc2420_set_auto_flushrx(0);
      leds_off(LEDS_GREEN);
      return;
    }
  }
}

/*--------------------------------------------------*/

#if 0
static void run_rssi_mirror(void)
{
  leds_on(LEDS_RED);
  for (;;) {

  }
}
#endif

/*---------------------------------------------------------------------------*/

char mode = 'N'; /* none */

void run_current_mode_loop()
{
  if (mode == 'P')
    run_packet_loop();
  else if (mode == 'R')
    run_rssi_loop();
  else if (mode == 'N')
    run_serial_loop();
}

/* Busy loop: we don't do any Contiki thread 'yield' thus the 'cc2420_process'
    has no way to run (nor any other contiki thread) 
*/
static void run_main_loop(void)
{
  my_init_uart0_2Mbps();
  //my_init_uart0();

  for (;;) {
    run_current_mode_loop();
    
    if (has_serial_command()) {
      uint8_t cmd_ok = 0;
      if (command_length == 0) {
	mode = 'N';
	//update_output_buffer(0, &mode, 1);
	cmd_ok = 1;
      } else {

	if (command_buffer[0] == '!') {
	  mode = 'N';
	  update_output_buffer(0, command_buffer, command_length);
	  cmd_ok = 1;
	} else if (command_length == 1) {
	  if (command_buffer[0] == 'P') {
	    mode = 'P';
	    update_output_buffer(0, &mode, 1);
	    cmd_ok = 1;
	  }  else if (command_buffer[0] == 'R') {
	    mode = 'R';
	    update_output_buffer(0, &mode, 1);
	    cmd_ok = 1;
	  }  else if (command_buffer[0] == 'V') {
	    append_output_buffer(SNIFFER_VERSION_MAJOR);
	    append_output_buffer(SNIFFER_VERSION_MINOR);
	    cmd_ok = 1;
	  }
	} else if (command_length == 2) {
	  if (command_buffer[0] == 'C') {
	    cc2420_set_channel(command_buffer[0]);
	    update_output_buffer(0, command_buffer, 2);
	    cmd_ok = 1;
	  } 
	}
	
      }
      if (!cmd_ok) {
	uint8_t error_code = 'E';
	update_output_buffer(0, &error_code, 1);
      }
      serial_command_state = StateNone;
    }
  }
}


/*---------------------------------------------------------------------------*/

void test_uart()
{ 
  my_uart0_init();
  for (;;) {
    my_uart0_write('A');
    my_uart0_write('B');
  } 
}

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  /* switch mac layer off, and turn radio on */
  //rime_mac->off(0);
  //cc2420_off();

  /* initial */
  my_timerb_init();

  /* set radio receiver on */
  cc2420_set_channel(26);

  /* main loop */
  run_main_loop();

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

AUTOSTART_PROCESSES(&init_process);

/*---------------------------------------------------------------------------*/
