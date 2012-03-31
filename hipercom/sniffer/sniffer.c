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
#include <string.h>

#include "net/netstack.h"

#include "net/rime.h"

#include "dev/cc2420.h"
#include "dev/cc2420_const.h"
#include "dev/spi.h"

/*---------------------------------------------------------------------------*/

#define USE_8MHZ_RESOLUTION 0

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

#define SNIFFER_VERSION_MAJOR 0
#define SNIFFER_VERSION_MINOR 2

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
  MY_LED_ON(MY_B); // XXX
  uint8_t data = my_uart_read();
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
  else if (!my_uart_has_data())
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
  my_uart_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
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
  MY_LED_ON(MY_R); // XXX
  for (;;) {
    MY_LED_TOGGLE(MY_R); // XXX
    MY_LED_TOGGLE(MY_G); // XXX
    while (!has_serial_command() && producer_offset != consumer_offset) {
      my_uart_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
      consumer_offset = (consumer_offset+1) & OUTPUT_BUFFER_OFFSET_MASK;
    }

    if (has_serial_command())
      return;
  }
}

/*---------------------------------------------------------------------------*/

extern int cc2420_set_channel(int c);
int current_channel = DEFAULT_CHANNEL;

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
void process_one_packet(void)
{
  my_time_t timestamp = my_get_clock();

  int hdrlen = COMMAND_ANSWER_HEADER_LEN;
  int len = cc2420_driver.read(receive_packet_buffer+hdrlen, PACKETBUF_SIZE);
  global_packet_counter++;
  receive_packet_buffer[0] = COMMAND_ANSWER_CODE1;
  receive_packet_buffer[1] = COMMAND_ANSWER_CODE2;
  receive_packet_buffer[2] = len+hdrlen-3;

  receive_packet_buffer[3] = 'p';
  receive_packet_buffer[4] = ((lost_packet & 0x7fu)<<1) | USE_8MHZ_RESOLUTION;

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

void process_one_sfd(void)
{
  my_time_t timestamp = my_get_clock();
  my_sfd_event_t* sfd_record = my_sfd_pop();
  if (sfd_record == NULL) {
    /* not possible ! */
    MY_LED_ON(MY_G);
    return;
  }

  int hdrlen = 6 + sizeof(sfd_record->event_time) + sizeof(timestamp);

  receive_packet_buffer[0] = COMMAND_ANSWER_CODE1;
  receive_packet_buffer[1] = COMMAND_ANSWER_CODE2;
  receive_packet_buffer[2] = hdrlen-3;
  receive_packet_buffer[3] = 's';
  receive_packet_buffer[4] = sfd_record->is_up;
  receive_packet_buffer[5] = 0;

  memcpy(receive_packet_buffer+6, 
	 &sfd_record->event_time, sizeof(sfd_record->event_time));
  memcpy(receive_packet_buffer+6+sizeof(sfd_record->event_time), 
	 &timestamp, sizeof(timestamp));
  
  if (!OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(hdrlen)) {
    lost_packet++;
    MY_LED_ON(MY_G);
  } else {
    int i;
    for (i=0; i<hdrlen; i++) {
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
  MY_LED_ON(MY_R);
#ifdef CONTIKI_TARGET_SKY
  // XXX" this is currently an hack because can't call in init for some reason
  cc2420_set_channel(current_channel); // XXX
  cc2420_set_no_addr_filter(); // XXX
#endif

  for (;;) {

    while (!my_sfd_is_empty())
      process_one_sfd();

    if (has_packet()) {
      last_packet_counter = cc2420_packet_counter;
      MY_LED_TOGGLE(MY_B);
      process_one_packet();
    }

    while (!has_packet() && my_sfd_is_empty() && !has_serial_command()
	   && producer_offset != consumer_offset) {
      my_uart_write(output_buffer[consumer_offset]);  // uart0_writeb(...);
      consumer_offset = (consumer_offset+1) & OUTPUT_BUFFER_OFFSET_MASK;
    }

    if (has_serial_command()) {
      MY_LED_OFF(MY_R | MY_B);
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
  MY_LED_ON(MY_G);
  command_answer_rssi_t answer;
  memset(&answer, 0xee, sizeof(answer));
  answer.command_code1 = COMMAND_ANSWER_CODE1;
  answer.command_code2 = COMMAND_ANSWER_CODE2;
  answer.command_len = sizeof(command_answer_rssi_t) - 3;
  answer.command_subcode = 'r';

#ifdef CONTIKI_TARGET_SKY
  // XXX" this is currently an hack because can't call in init for some reason
  cc2420_set_channel(current_channel); // XXX
  cc2420_set_no_addr_filter(); // XXX
#endif
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
    if (!MY_IS_UART_BUSY && producer_offset != consumer_offset)
      send_one_byte_buffer_to_serial();

    /* if output buffer is almost full, write at least a block of bytes  */
    while (!OUTPUT_BUFFER_HAS_FREE_SPACE_WITH_RESERVE_FOR(sizeof(answer))
	   && !has_serial_command())
      send_one_byte_buffer_to_serial();

    if (has_serial_command()) {
      cc2420_set_auto_flushrx(0);
      MY_LED_OFF(MY_G);
      return;
    }
  }
}

/*--------------------------------------------------*/

static void run_rssi_dac(void)
{
#ifdef CONTIKI_TARGET_SKY
  // XXX" this is currently an hack because can't call in init for some reason
  cc2420_set_channel(current_channel); // XXX
  cc2420_set_no_addr_filter(); // XXX
#endif
  cc2420_set_auto_flushrx(1); /* disable receiving packets by flushing them */

  /* empty first the serial buffer */
  while (producer_offset != consumer_offset)
    send_one_byte_buffer_to_serial();

  leds_on(LEDS_GREEN);

  /* disable receiving packets by immediately flushing them */
  cc2420_set_auto_flushrx(1); 

  my_dac_init();

  while (!has_serial_command()) {
    uint8_t rssi = (cc2420_rssi() + 55);
    my_dac_set_output(rssi << 4);

    if (rssi > 40) MY_LED_ON(MY_B);
    else MY_LED_OFF(MY_B);
  }

  leds_off(LEDS_GREEN);
  leds_off(LEDS_BLUE);
  cc2420_set_auto_flushrx(0);
}

/*---------------------------------------------------------------------------*/

#ifndef DEFAULT_SNIFFER_MODE
char mode = 'N'; /* none */
#else
char mode = DEFAULT_SNIFFER_MODE;
#endif

void run_current_mode_loop()
{
  if (mode == 'P')
    run_packet_loop();
  else if (mode == 'R')
    run_rssi_loop();
  else if (mode == 'N')
    run_serial_loop();
  else if (mode == 'D')
    run_rssi_dac();
}

/* Busy loop: we don't do any Contiki thread 'yield' thus the 'cc2420_process'
    has no way to run (nor any other contiki thread) 
*/
static void run_main_loop(void)
{
  my_uart_init();

  for (;;) {
    run_current_mode_loop();
    
    if (has_serial_command()) {
      uint8_t cmd_ok = 0;
      if (command_length == 0) {
	mode = 'N';
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
	  }  else if (command_buffer[0] == 'H') {
	    mode = 'H';
	    update_output_buffer(0, &mode, 1);
	    my_uart_switch_to_2Mbps();
	    cmd_ok = 1;
	  }  else if (command_buffer[0] == 'D') {
	    mode = 'D';
	    update_output_buffer(0, &mode, 1);
	    cmd_ok = 1;
	  }  else if (command_buffer[0] == 'V') {
	    append_output_buffer(SNIFFER_VERSION_MAJOR);
	    append_output_buffer(SNIFFER_VERSION_MINOR);
	    cmd_ok = 1;
	  }
	} else if (command_length == 2) {
	  if (command_buffer[0] == 'C') {
	    current_channel = command_buffer[1];
	    cc2420_set_channel(current_channel);
	    update_output_buffer(0, command_buffer, 2);
	    cmd_ok = 1;
	  } 
	}
	
      }
      if (!cmd_ok) {
	uint8_t error_code = 'E';
	update_output_buffer(0, &error_code, 1);
	//update_output_buffer(0, command_buffer, command_length);
      }
      serial_command_state = StateNone;
    }
  }
}


/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  watchdog_stop();

  /* (could switch mac layer off, and turn radio off) */
  //rime_mac->off(0);
  //cc2420_off();

  /* initialisation */
  my_timerb_init(USE_8MHZ_RESOLUTION); /* 32 kHz resol. clock (KiHz) or 8 MHz*/
  my_sfd_init();


#ifndef CONTIKI_TARGET_SKY
  /* set radio receiver on */
  cc2420_on();
  cc2420_set_channel(current_channel);
  cc2420_set_no_addr_filter();
#else
#warning "*** XXX - NOT CONFIGURING RADIO"
  printf("!!! XXX\n");
#endif

  /* main loop */
  MY_LED_OFF(MY_R | MY_G | MY_B);
  run_main_loop();

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

AUTOSTART_PROCESSES(&init_process);

/*---------------------------------------------------------------------------*/
