/*---------------------------------------------------------------------------
 *                         Test of standard contiki clock
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
  Typical results  TelosB ---- AND OLD MSPGCC

  (not WITH_STRUCT_TIME, EXTRA_MULT=2)
  sp=35/31 cycles - my_get_time:31 
  sp=36/31 cycles - clock_time:0 
  sp=36/31 cycles - clock_seconds:0 
  sp=23/18 cycles - clock_fine:0 
  sp=146/139 cycles - full_time:2

  (not WITH_STRUCT_TIME, EXTRA_MULT=1)
  sp=226/207 cycles - full_time:2       -> 28 usec

  (WITH_STRUCT_TIME, EXTRA_MULT=2)
  sp=200/198 cycles - full_time:0

  (not WITH_STRUCT_TIME, EXTRA_MULT=2 and 'return result/EXTRA_MULT;' )
  sp=153/150 cycles - full_time:1

  ---------- with new MSPGCC

  (not WITH_STRUCT_TIME, EXTRA_MULT=2 and 'return result/EXTRA_MULT;' )
  

 */

#include <stdio.h>

#include "contiki.h"
#include "clock.h"

extern rtimer_clock_t clock_counter(void);

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*---------------------------------------------------------------------------*/

#define TEST_TIME_FMT "%lld"
#define TICK_TO_RTIMER  (RTIMER_ARCH_SECOND / CLOCK_SECOND)
#define EXTRA_MULT 2

//#define WITH_SECONDS

//#define WITH_STRUCT_TIME

#ifdef WITH_STRUCT_TIME

typedef union u_test_time_t {
  struct { uint32_t high_sec; uint16_t low_sec; uint16_t frac; } part;
  long long value;
} test_time_t;


#define TT(x) ((x).value)

#if (RTIMER_ARCH_SECOND*EXTRA_MULT) != 65536
#error "bad choice of parameters"
#endif

// for some reason the value is always 0
#warning "this code is not actually working"
static inline test_time_t get_full_time(void)
{
  clock_time_t tick1, tick2;
  unsigned short fine1;
  unsigned long second1;
  do {
    tick1 = clock_time();
    fine1 = clock_fine();
    second1 = clock_seconds();
    tick2 = clock_time();
  } while (tick1 != tick2);

  test_time_t result; 
  result.part.high_sec = second1 >> 16;
  result.part.low_sec = second1 & 0xffffu;
  result.part.frac
    = (tick1 % CLOCK_CONF_SECOND) * (TICK_TO_RTIMER*EXTRA_MULT)
    + (clock_fine() % TICK_TO_RTIMER)*EXTRA_MULT;
  return result;
}


#else /* WITH_STRUCT_TIME -------------------- */

#define TT(x) (x)

typedef long long test_time_t;

void printf_time(test_time_t t)
{ 
  int i;
  test_time_t t_copy = t;

#if 0
  unsigned long t1 = t & 0xfffffffful;
  t >>= 32;
  long t2 = t;
  printf("[%lx:%lx]", t2, t1);
#endif

#if 0
  unsigned char* ptr = (unsigned char*)&t_copy;
  printf("[");
  for (i=0;i<8;i++)
    printf(":%x", ptr[i]);
  printf("]");
#endif

  char buffer[24];
  buffer[sizeof(buffer)-1] = 0;
  i = sizeof(buffer)-2;

  if (t < 0) {
    printf("-");
    t = -t;
  }
  while(t!=0 && i >=0) {
    buffer[i] = '0' + (t % (unsigned long long)10);
    t /= 10;
    i --;
  }
  printf("%s", buffer+i+1);
}

#if 0

static inline test_time_t get_full_time(void)
{
  clock_time_t tick1, tick2;
  unsigned short fine1;
#ifdef WITH_SECONDS
  unsigned long second1;
#endif
  do {
    tick1 = clock_time();
    fine1 = clock_fine();
#ifdef WITH_SECONDS
    second1 = clock_seconds();
#endif
    tick2 = clock_time();
  } while (tick1 != tick2);

  tick1 += (1ul<<30);
  tick2 += (1ul<<30);

#if 0
  test_time_t result
    = (tick1 % CLOCK_CONF_SECOND) * (TICK_TO_RTIMER*EXTRA_MULT)
    + (clock_fine() % TICK_TO_RTIMER) * EXTRA_MULT;
  result += (((test_time_t)second1) * ((test_time_t)RTIMER_ARCH_SECOND
				       *(unsigned long)EXTRA_MULT));
  return result/EXTRA_MULT;
#endif 

#if 1
#ifdef WITH_SECONDS
  test_time_t result = ((test_time_t) second1)
    *(RTIMER_ARCH_SECOND*(unsigned long)EXTRA_MULT);
  result /= EXTRA_MULT;
  result += (tick1 % CLOCK_CONF_SECOND) * (TICK_TO_RTIMER)
    + (fine1 % TICK_TO_RTIMER);
  return result;
#else
  test_time_t result = tick1;
  result *= TICK_TO_RTIMER;
  //test_time_t result = 0;
  //result += (unsigned long long)(fine1 % TICK_TO_RTIMER);
  test_time_t fine_result = fine1 % TICK_TO_RTIMER;
  printf_time(fine_result);
  result += fine_result;
  return result;
  //return ((test_time_t)tick1) * ((test_time_t)TICK_TO_RTIMER)
  //+ (clock_fine() % TICK_TO_RTIMER);
#endif

#endif

}


#endif

static test_time_t get_full_time(void)
{
  clock_time_t tick1, tick2;
  unsigned short fine1 = 0;
  do {
    tick1 = clock_time();
    fine1 = clock_fine();
    tick2 = clock_time();
  } while (tick1 != tick2);


  static test_time_t result; result = tick1;
  result *= TICK_TO_RTIMER;
  result += fine1;

  // test_time_t result = fine1;
  // result += ((long long)(tick1)) * ((long long)(TICK_TO_RTIMER));

  return result;
}




#endif /* WITH_STRUCT_TIME */

/*---------------------------------------------------------------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);


#define EVAL_SPEED(x,y,z)			\
  do { \
     uint16_t tbr1 = TBR; \
     x; \
     uint16_t tbr2 = TBR; \
     y; \
     uint16_t tbr3 = TBR; \
     printf("sp=%u/%u cycles", (unsigned short)(tbr3-tbr2), \
	    (unsigned short)(tbr2-tbr1));		     \
     z; \
  } while (0)
  

PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  my_timerb_init(1); /* set up timerb with 8Mhz source and int. counter */

  printf("sizeof(clock_time_t)=%d, sizeof(rtimer_clock_t)=%d\n",
	 sizeof(clock_time_t), sizeof(rtimer_clock_t));

  clock_set(0 /*clock*/, 100 /*TAR*/); 

  watchdog_stop();

  int i=0;
  for (i=0; i<10; i++) {
    EVAL_SPEED( clock_time_t t1 = clock_time(), 
	        clock_time_t t2 = clock_time(),
		printf(" - clock_time:%lu \n", (clock_time_t)(t2-t1) ));
    EVAL_SPEED( unsigned long t1 = clock_seconds(), 
	        unsigned long t2 = clock_seconds(),
		printf(" - clock_seconds:%lu \n", (unsigned long)(t2-t1) ));
    EVAL_SPEED( uint16_t t1 = clock_fine(), 
	        uint16_t t2 = clock_fine(),
		printf(" - clock_fine:%u \n", (uint16_t)(t2-t1) ));
    EVAL_SPEED( test_time_t t1 = get_full_time(),
		test_time_t t2 = get_full_time(), 
		long long dt = TT(t2) - TT(t1);
		printf( " - full_time:"); printf_time(dt); printf("\n")
		);
    EVAL_SPEED( my_time_t t1 = my_get_clock(),
		my_time_t t2 = my_get_clock(), 
		printf( " - my_get_time:%lu \n", (my_time_t)(t2-t1))  );
  }
  
  long long lli = (1<<14) + 0x11111;
  printf("lli=%llx ", lli);
  lli *= (1<<14) + 0*0x11111;
  printf("lli=%llx ", lli);
  lli *= (1<<14) + 0*0x11111;
  printf("lli=%llx ", lli);
  lli *= (1<<14) + 0*0x11111;
  printf("lli=%llx ", lli);
  lli >>= 32;
  printf("lli=%llx\n", lli);


  test_time_t value = clock_time();
  unsigned short value_low = clock_fine();
  value *= TICK_TO_RTIMER;
  value += value_low % TICK_TO_RTIMER;
  printf("llv=");
  printf_time(value);
  printf("\n");

  for (;;) {
    //clock_time_t time0 = clock_time();
    uint16_t tar1 = TAR;
    rtimer_clock_t counter1 = clock_counter();
    uint16_t fine1 = clock_fine();
    unsigned long sec1 = clock_seconds();
    test_time_t full_time1 = get_full_time();
    clock_time_t time1 = clock_time();

    clock_time_t time2 = clock_time();
    test_time_t full_time2 = get_full_time();
    uint16_t tar2 = TAR;
    rtimer_clock_t counter2 = clock_counter();
    uint16_t fine2 = clock_fine();
    unsigned long sec2 = clock_seconds();

    if (time1 != time2) {
      printf("ft="); printf_time(TT(full_time1));
      printf("/"); printf_time(TT(full_time2));
      printf(" time=%lu/%lu tar=%u/%u count=%u/%u fine=%u/%u sec=%lu/%lu",
	     time1, time2, tar1, tar2, counter1, counter2, fine1, fine2, 
	     sec1,sec2);

      tar1 = TBR;
      full_time1 = get_full_time();
      tar2 = TBR;
      full_time2 = get_full_time();

      printf(" cycl:%u %lld\n", (uint16_t)(tar2-tar1), 
	     (TT(full_time2)-TT(full_time1)));
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
