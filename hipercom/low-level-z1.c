/*---------------------------------------------------------------------------
 *                         Low Level Tools
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
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

void my_uart0_init(void)
{
  IE2 &= ~UCA0TXIE;               /* Disable UCA0 TX interrupt */
  IE2 &= ~UCA0RXIE;               /* Disable UCA0 RX interrupt */
  IFG2 &= ~UCA0RXIFG;             /* Clear pending interrupts */
  IFG2 &= ~UCA0TXIFG;
}

void my_uart0_init_2Mbps()
{
  UCA0BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver! */
  my_uart0_init();
}

void my_uart0_switch_to_2Mbps()
{
  UCA0BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver */
}

static inline void my_uart0_write(uint8_t data)
{
  while((UCA0STAT & UCBUSY));
  UCA0TXBUF = data;
}

static inline uint8_t my_uart0_read(void)
{ return UCA0RXBUF; }

static inline uint8_t my_uart0_has_data(void)
{ return IFG2 & UCA0RXIFG; }

#define MY_IS_UART0_BUSY ((UCA0STAT & UCBUSY))

/*---------------------------------------------------------------------------*/

#if 0 
static void my_set_channel(int channel)
{
  int f;
  f = 5*(channel-11) + 352;
  CC2420_WRITE_REG(CC2420_FSCTRL, f);
  CC2420_STROBE(CC2420_SRXON);
}
#endif

/*---------------------------------------------------------------------------*/
