/*---------------------------------------------------------------------------
 * Test for cycle counter using TimerB
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

/*---------------------------------------------------------------------------*/

#include "../low-level.c"

/*--------------------*/

PROCESS(init_process, "init");
AUTOSTART_PROCESSES(&init_process);

#define NOP1     asm("nop;");
#define NOP4   NOP1 NOP1 NOP1 NOP1
#define NOP16  NOP4 NOP4 NOP4 NOP4
#define NOP64  NOP16 NOP16 NOP16 NOP16
#define NOP256 NOP64 NOP64 NOP64 NOP64
#define NOP1024 NOP256 NOP256 NOP256 NOP256

PROCESS_THREAD(init_process, ev, data)
{
  PROCESS_BEGIN();

  //my_timerb_init(1); /* set up timerb with 8Mhz source and int. counter */
  my_timerb_init(0); /* set up timerb with 32Khz source and int. counter */

  for (;;) {
    uint32_t t1 = my_get_clock();
    uint32_t t2 = my_get_clock();
    uint32_t t3 = my_get_clock();
    NOP1024;
    uint32_t t4 = my_get_clock();
    uint32_t t5 = my_get_clock();

    printf("t2-t1=%lu t3-t2=%lu t4-t3=%lu t5-t4=%lu [cycles]\n",
	   t2-t1, t3-t2, t4-t3, t5-t4);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
