/*---------------------------------------------------------------------------
 * Low Level Tools
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
   Low level tools for misc. system hacking: 
   just #include this file in your .c file
   (for gcc, "static inline" should be as fast as a macro)
*/

/*---------------------------------------------------------------------------*/

#if 0
  /* XXX: we need to test the right flags to check which interrupt was called */
  switch ( __even_in_range(TBIV, 14)) {
  case 2:
    my_timerb_count ++;
    break
  default:
    break;
  }
}
#endif

#if 0
http://old.nabble.com/__even_in_range-equivalent--td15124831.html

interrupt (TIMERA1_VECTOR) __attribute__ ((naked)) tax_int(void) { 
     //demux timer interrupts 
     asm("add     %0, r0                  ; TAirq# + PC"::"m" (TAIV)); 
     asm("reti                            ; CCR0 - no source"); 
     asm("jmp ccr1                        ; CCR1"); 
     asm("jmp ccr2                        ; CCR2"); 
     asm("reti                            ; CCR3 - no source"); 
     asm("reti                            ; CCR4 - no source"); 
     asm("        br #INT_TimerA_TAOVER   ; TAOVER (follows directly)"); 
     asm("ccr1:   br #INT_TimerA_CCR1"); 
     asm("ccr2:   br #INT_TimerA_CCR2"); 
} 

//interrupt (TIMERB2_VECTOR)
//my_timerb4_interrupt (void)
//{
//
//}

#endif

/*---------------------------------------------------------------------------*/

void my_uart_init(void)
{
  IE2 &= ~UCA0TXIE;               /* Disable UCA0 TX interrupt */
  IE2 &= ~UCA0RXIE;               /* Disable UCA0 RX interrupt */
  IFG2 &= ~UCA0RXIFG;             /* Clear pending interrupts */
  IFG2 &= ~UCA0TXIFG;
}

void my_uart_init_2Mbps()
{
  UCA0BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver! */
  my_uart_init();
}

void my_uart_switch_to_2Mbps()
{
  UCA0BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver */
}

static inline void my_uart_write(uint8_t data)
{
  while((UCA0STAT & UCBUSY));
  UCA0TXBUF = data;
}

static inline uint8_t my_uart_read(void)
{ return UCA0RXBUF; }

static inline uint8_t my_uart_has_data(void)
{ return IFG2 & UCA0RXIFG; }

#define MY_IS_UART_BUSY ((UCA0STAT & UCBUSY))

/*---------------------------------------------------------------------------*/

void my_io_init(void)
{
  /* Set up P4 output */
  P4SEL &= ~(1<<2); // P4.2    is selected as I/O
  P4DIR |= (1<<2);  // P4.2    is output
  P4OUT |= (1<<2); // P4.2    set to 1
}

static inline void my_io_set_output(unsigned int value)
{
  if (value == 0) P4OUT |= (1<<2);
  else P4OUT &= ~(1<<2);
}

static inline void my_io_toggle(void)
{ P4OUT ^= (1<<2); }

/*---------------------------------------------------------------------------*/

#if 0 
/* copied from elsewhere */
static void my_set_channel(int channel)
{
  int f; 
  f = 5*(channel-11) + 352;
  CC2420_WRITE_REG(CC2420_FSCTRL, f);
  CC2420_STROBE(CC2420_SRXON);
}
#endif

/*---------------------------------------------------------------------------*/
