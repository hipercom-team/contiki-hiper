/*---------------------------------------------------------------------------
 *               Test for ctimer waiting for 0 second
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
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

uint32_t clock1, clock2, clock3, clock4, clock5, clock6, clock7, clock8, clock9;
uint16_t timer1, timer2, timer3, timer4, timer5, timer6, timer7, timer8, timer9;
static struct ctimer callback_timer;
struct rtimer callback_rtimer;

void display_result()
{
  printf("----------------------\n");
  printf("Starting t=%lu TAR=%u\n", clock1, timer1);
  printf("After waiting etimer_set(...,0) t=%lu TAR=%u\n", clock2, timer2);
  printf("After waiting etimer_set(...,0) t=%lu TAR=%u\n", clock3, timer3);
  printf("After waiting ctimer_set(...,0) t=%lu TAR=%u\n", clock4, timer4);
  printf("After waiting ctimer_set(...,0) t=%lu TAR=%u\n", clock5, timer5);
  printf("After waiting ctimer_set(...,1) t=%lu TAR=%u\n", clock6, timer6);
  printf("After waiting ctimer_set(...,1) t=%lu TAR=%u\n", clock7, timer7);
  printf("After waiting rtimer_set(...,1) t=%lu TAR=%u\n", clock8, timer8);
  printf("After waiting rtimer_set(...,3) t=%lu TAR=%u\n", clock9, timer9);

}

void callback_func1(void* callback_arg);
void callback_func2(void* callback_arg);
void callback_func3(void* callback_arg);
void callback_func4(void* callback_arg);
void callback_func5(void* callback_arg);
void callback_func6(void* callback_arg);

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

void callback_func5(void* callback_arg)
{
  clock8 = my_get_clock();
  timer8 = TAR;
  rtimer_set(&callback_rtimer, RTIMER_NOW()+3, 
	     0/*unused*/, callback_func6, NULL);
}


void callback_func6(void* callback_arg)
{
  clock9 = my_get_clock();
  timer9 = TAR;
  display_result();
}


PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  my_timerb_init(1); /* set up timerb with 8Mhz source and int. counter */

  clock1 = my_get_clock();
  timer1 = TAR;

  static struct etimer delay;
  static int i;

  //for (i=0;i<10;i++) {
  etimer_set(&delay, 0);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay));
  
  clock2 = my_get_clock();
  timer2 = TAR;
  
  etimer_set(&delay, 0);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay));
  
  clock3 = my_get_clock();
  timer3 = TAR;
  
  ctimer_set(&callback_timer, 0, callback_func1, NULL);
  //}

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
