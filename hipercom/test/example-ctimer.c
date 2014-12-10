/*---------------------------------------------------------------------------
 * Test for ctimer waiting for 0 second
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

#include <stdio.h>
#include "contiki.h"
#include "timer.h"
#include "rtimer.h"

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*--------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

uint32_t clock1, clock2, clock3, clock4, clock5, clock6, clock7, clock8, clock9=0,clock10;
uint16_t timer1, timer2, timer3, timer4, timer5, timer6, timer7, timer8, timer9=0,timer10;
static struct ctimer callback_timer;
struct rtimer callback_rtimer;
struct rtimer callback_rtimer2;

#define PRINTDIFF(pre,idx,idxref)					\
  printf(pre " t=%lu (dt=%lu) TAR=%u[%u] (dT=%u)\n",			\
	 clock##idx, (clock##idx-clock##idxref),			\
	 timer##idx, (timer##idx & 0xffu),				\
	 (timer##idx - timer##idxref) )

void display_result()
{
  printf("----------------------\n");
  printf("Starting t=%lu TAR=%u\n", clock1, timer1);
  //printf("After waiting etimer_set(...,0) t=%lu TAR=%u\n", clock2, timer2);
  PRINTDIFF("After waiting etimer_set(...,0)", 2, 1);
  PRINTDIFF("After waiting etimer_set(...,1)", 3, 2);
  PRINTDIFF("After waiting ctimer_set(...,0)", 4, 3);
  PRINTDIFF("After waiting ctimer_set(...,0)", 5, 4);
  PRINTDIFF("After waiting ctimer_set(...,1)", 6, 5);
  PRINTDIFF("After waiting ctimer_set(...,1)", 7, 6);
  PRINTDIFF("After waiting rtimer_set(...,1)", 8, 7);
  PRINTDIFF("After waiting rtimer_set(...,3)", 9, 8);
  PRINTDIFF("After waiting rtimer_set(...,3)", 10, 8);
}

void callback_func1(void* callback_arg);
void callback_func2(void* callback_arg);
void callback_func3(void* callback_arg);
void callback_func4(void* callback_arg);
void callback_func5(struct rtimer *t, void* callback_arg);
void callback_func6(struct rtimer *t, void* callback_arg);
void callback_func7(struct rtimer *t, void* callback_arg);

void callback_func1(void* callback_arg)
{
  clock4 = my_get_clock();
  timer4 = TAR;
  ctimer_set(&callback_timer, 0, callback_func2, NULL);
}

void callback_func2(void* callback_arg)
{
  clock5 = my_get_clock();
  timer5 = TAR;
  ctimer_set(&callback_timer, 1, callback_func3, NULL);
}

void callback_func3(void* callback_arg)
{
  clock6 = my_get_clock();
  timer6 = TAR;
  ctimer_set(&callback_timer, 1, callback_func4, NULL);
}

void callback_func4(void* callback_arg)
{
  clock7 = my_get_clock();
  timer7 = TAR;
  rtimer_set(&callback_rtimer, RTIMER_NOW()+1, 0/*unused*/, 
	     callback_func5, NULL);
}

void callback_func5(struct rtimer *t, void* callback_arg)
{
  clock8 = my_get_clock();
  timer8 = TAR;
  int v1 = rtimer_set(&callback_rtimer2, RTIMER_NOW()+10, 
       0/*unused*/, callback_func7, NULL);
  int v2 = rtimer_set(&callback_rtimer, RTIMER_NOW()+2,
  	     0/*unused*/, callback_func6, NULL);
  printf("%d %d\n", v1,v2);
}

void callback_func6(struct rtimer *t, void* callback_arg)
{
  clock9 = my_get_clock();
  timer9 = TAR;
  display_result();
  process_poll(&init_process); /* wake-up init_process */
}

void callback_func7(struct rtimer *t, void* callback_arg)
{
  clock10 = my_get_clock();
  timer10 = TAR;
  display_result();
  process_poll(&init_process); /* wake-up init_process */
}


PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  watchdog_stop();

  my_timerb_init(1); /* set up timerb with 8Mhz source and int. counter */


  static struct etimer delay;
  static int i;

  for (i=0;i<10;i++) {
    while( (TAR&0xff)/10 != i) ;

    clock1 = my_get_clock();
    timer1 = TAR;
    
    etimer_set(&delay, 0);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay));
    
    clock2 = my_get_clock();
    timer2 = TAR;
    
    etimer_set(&delay, 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay));
    
    clock3 = my_get_clock();
    timer3 = TAR;
    
    ctimer_set(&callback_timer, 0, callback_func1, NULL);

    PROCESS_WAIT_EVENT(); /* sleep until waken-up */
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
