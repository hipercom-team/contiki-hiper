/*---------------------------------------------------------------------------
 * Accel. stuff
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


/*
  copied from 'opera-data-packet.c'
  copied from 'contiki-opera.c'
 */

int double_tap_detected = 0;
int tap_detected = 0;

/*--------------------------------------------------*/

#define GRAVITY_THRESHOLD 170

char getGravity(void)
{
#ifdef CONTIKI_TARGET_Z1
  int16_t x,y,z;
  x = accm_read_axis(X_AXIS);
  y = accm_read_axis(Y_AXIS);
  z = accm_read_axis(Z_AXIS);
  //printf("x: %d y: %d z: %d\n", x, y, z);

  if (x > GRAVITY_THRESHOLD) return 'x';
  else if (x < -GRAVITY_THRESHOLD) return 'X';
  if (y > GRAVITY_THRESHOLD) return 'y';
  else if (y < -GRAVITY_THRESHOLD) return 'Y';
  if (z > GRAVITY_THRESHOLD) return 'z';
  else if (z < -GRAVITY_THRESHOLD) return 'Z';
#endif /* CONTIKI_TARGET_Z1 */
  return '.';
}

/*--------------------------------------------------*/

#if 0
PROCESS(data_acquire_thread, "");
PROCESS_THREAD(data_acquire_thread, ev, data)
{
  static int periodic_send = 0;
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND/2);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  for (;;) {
    PROCESS_WAIT_EVENT();

    if (double_tap_detected) {
      if (is_pointing_down())
	periodic_send = !periodic_send;
      else tap_detected = 1;
    }
    if (periodic_send || tap_detected)
      send_accel_data();
    double_tap_detected = 0;
    tap_detected = 0;

    if (etimer_expired(&et) && periodic_send)
      etimer_set(&et, CLOCK_SECOND/3);
  }
  PROCESS_END();
}
#endif

/*--------------------------------------------------*/

void accm_tap_cb(u8_t reg)
{
  if (reg & ADXL345_INT_DOUBLETAP)
    double_tap_detected = 1;
  else if (reg & ADXL345_INT_TAP)
    tap_detected = 1;
#if 0
  if (node_id != 1)
    process_poll(&data_acquire_thread);
#endif
}

/*--------------------------------------------------*/

void accel_init(void)
{
#if 0
  process_start(&data_acquire_thread, NULL);
#endif
  /* Start and setup the accelerometer with default values, eg no int. */
  //accm_init(); // already done in contiki-z1-main.c
  getGravity();
  ACCM_REGISTER_INT1_CB(accm_tap_cb);
  //ACCM_REGISTER_INT2_CB(accm_tap_cb);
  accm_set_irq(ADXL345_INT_DOUBLETAP + ADXL345_INT_TAP, 0);
}

/*--------------------------------------------------*/

#define CH_OFFSET 2
#define MIN_EVENT_DELAY (CLOCK_SECOND/2)

static clock_time_t last_accel_event_clock = 0;
static int current_channel = CHANNEL;
char last_gravity = '?';

void check_accel_event(void)
{
#if 0
  if (!double_tap_detected && !tap_detected)
    return;
#endif
  char gravity = getGravity();
  //printf("%c", gravity);
  if (gravity != 'z' && gravity != 'Z')
    return;
  if (last_gravity == gravity)
    return;

  printf("[TAP] %d %d %c\n", double_tap_detected, tap_detected, gravity);

  clock_time_t current_clock = clock_time();
  if (current_clock - last_accel_event_clock < MIN_EVENT_DELAY)
    return;

  /* change channel */
  int new_channel = 3*((current_channel - 11) / 3 + 1)+11+CH_OFFSET;
  if (new_channel > 26)
    current_channel = 11 + CH_OFFSET;
  else current_channel = new_channel;

  printf("channel=%d\n", current_channel);
  cc2420_set_channel(current_channel);
  double_tap_detected = 0;
  tap_detected = 0;
  last_gravity = gravity;
}

/*--------------------------------------------------*/
