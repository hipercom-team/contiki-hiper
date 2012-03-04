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

/* See: 
   platform/z1/dev/cc2420-arch.c -- (cc2420_arch_init(...))
   cpu/msp430/cc2420-arch-sfd.c  -- (cc2420_arch_sfd_init(...))
   contiki/cpu/cc2430/dev/uart.c

   http://www.ti.com/lit/ds/symlink/msp430f2617.pdf
   http://www.ti.com/lit/ug/slau049f/slau049f.pdf
 */

volatile uint16_t my_timerb_count = 0;

#define MY_TICKS_PER_SECOND (244ul*0x8000ul)

/* hijack the timerb interrupt */
interrupt (TIMERB1_VECTOR)
my_timerb_interrupt (void)
{
  int tbiv;

  tbiv = TBIV;
  /* XXX: we need to test the right flags to check which interrupt was called */
  my_timerb_count ++;
}

/* derived from cc2420_arch_sfd_init */
void my_timerb_init(void)
{
  /* Need to select the special function! */
  // P4SEL = BV(CC2420_SFD_PIN); // <--- NOT!!!
  
  /* start timer B - 32768 ticks per second */
  // TBCTL = TBSSEL_1 | TBCLR;

  /* start timer B - 8000000 ticks per second */
  TBCTL = TBSSEL_2 | TBCLR; //| (divisor<<6)

  
  /* CM_3 = capture mode - capture on both edges */
  TBCCTL1 = CM_3 | CAP | SCS;
  TBCCTL1 |= CCIE;

  /* Start Timer_B in continuous mode. */
  TBCTL |= MC1;

  /* reset time */
  my_timerb_count = 0;
  TBR = 0;

  TBCTL |= TBIE; /* enable overflow interrupt */
}

typedef uint32_t my_time_t;

static my_time_t my_get_clock(void)
{
  for(;;) {
    uint16_t clock_high1 = my_timerb_count;
    uint16_t clock_low = TBR;
    uint16_t clock_high2 = my_timerb_count;
    if (clock_high1 == clock_high2) 
      return (((uint32_t)clock_high1) << 16) + clock_low;
  }
}

#define MY_GET_CLOCK_U16 (TBR)

/*---------------------------------------------------------------------------*/

void my_uart0_init(void)
{
  //UCA0BR0 = 0x10;                 /* 8MHz/500000 = 16 = 0x10 */
  //UCA0BR0 = 0x8;                  /* 8MHz/1000000 = 8 -> 1 Mbps */
  //#if SERIAL_SPEED == 2000000
  //UCA0BR0 = 0x5;                  /* 8MHz/1600000 = 5 -> 1.6 Mbps 
  //but ok for 2 Mbps on receiver! */
  //#endif

  IE2 &= ~UCA0TXIE;               /* Disable UCA0 TX interrupt */
  IE2 &= ~UCA0RXIE;               /* Disable UCA0 RX interrupt */
  IFG2 &= ~UCA0RXIFG;             /* Clear pending interrupts */
  IFG2 &= ~UCA0TXIFG;
}

void my_init_uart0_2Mbps()
{
  UCA0BR0 = 0x5;                  /* 8MHz/1600000 = 5 -> 1.6 Mbps 
                                     but ok for 2 Mbps on receiver! */
  my_uart0_init();
}

void my_uart0_switch_to_2Mbps()
{
  UCA0BR0 = 0x5;                  /* 8MHz/1600000 = 5 -> 1.6 Mbps 
                                     but ok for 2 Mbps on receiver! */
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
 
