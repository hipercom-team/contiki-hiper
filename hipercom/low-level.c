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

/* See: 
   platform/z1/dev/cc2420-arch.c -- (cc2420_arch_init(...))
   cpu/msp430/cc2420-arch-sfd.c  -- (cc2420_arch_sfd_init(...))
   contiki/cpu/cc2430/dev/uart.c

   See doc:
   http://www.ti.com/lit/ds/symlink/msp430f2617.pdf
   http://www.ti.com/lit/ug/slau144i/slau144i.pdf
   http://zolertia.sourceforge.net/wiki/images/e/e8/Z1_RevC_Datasheet.pdf
   http://jmfriedt.free.fr/tp3_2009.pdf
 */

/*---------------------------------------------------------------------------*/

#define MY_LEDS_DIR      P5DIR
#define MY_LEDS_SEL      P5SEL
#define MY_LEDS_OUT      P5OUT

#define MY_LED_ON(x)     (MY_LEDS_OUT &= ~(x))
#define MY_LED_OFF(x)    (MY_LEDS_OUT |= (x))
#define MY_LED_TOGGLE(x) (MY_LEDS_OUT ^= (x))

#define MY_R        0x10
#define MY_G        0x40
#define MY_B        0x20

/*---------------------------------------------------------------------------*/

volatile uint16_t my_timerb_count = 0;

#define MY_TICKS_PER_SECOND (244ul*0x8000ul)

/*--------------------------------------------------*/

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



/* derived from cc2420_arch_sfd_init */
void my_timerb_init(uint8_t use8Mhz)
{
  /* start timer B - ~8000000 ticks per second */
  if (use8Mhz) {
    TBCTL = 
      TBSSEL_2 /* source = SMCLK */
      | TBCLR; /* this resets TBR, the clock divisor and count direction */
    //| (divisor<<6)
  } else {
    TBCTL = 
      TBSSEL_1 /* source = ACLK */
      | TBCLR; /* this resets TBR, the clock divisor and count direction */
    //| (divisor<<6)
  }
   
  /* Compare  1 */
  TBCCTL1 = 
    CM_3    /* capture on both rising and falling edges */
    | CAP   /* capture mode (by opposition to "compare mode") */
    | SCS   /* synchronous capture source */
    | CCIE; /* capture/compare interrupt enable */

  /* Start Timer_B in continuous mode. */
  TBCTL |= MC_2; /* continuous mode */

  /* reset time */
  my_timerb_count = 0;
  TBR = 0;

  TBCTL |= TBIE; /* timer B interrupt enable */
}

typedef uint32_t my_time_t;

static inline my_time_t my_get_clock(void)
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

#define MY_SFD_RECORD_LOG2_SIZE 4 /* 16 value */
#define MY_SFD_RECORD_OFFSET_MASK (BV(MY_SFD_RECORD_LOG2_SIZE)-1)

volatile uint8_t my_sfd_first = 0;
volatile uint8_t my_sfd_last  = 0;

typedef struct {
  my_time_t event_time;
  uint8_t   is_up;
} my_sfd_event_t;

my_sfd_event_t my_sfd_record[(1<<MY_SFD_RECORD_LOG2_SIZE)];

void my_sfd_init(void)
{
  my_sfd_first = 0;
  my_sfd_last = 0;
  P4SEL |= BV(CC2420_SFD_PIN); /* activate SFD interrupts */
}

static inline uint8_t my_sfd_is_empty()
{ return (my_sfd_first == my_sfd_last); }

static inline my_sfd_event_t* my_sfd_peek()
{ 
  if (my_sfd_is_empty())
    return NULL;
  else return &(my_sfd_record[my_sfd_first]);
}

static inline my_sfd_event_t* my_sfd_pop()
{
  if (my_sfd_is_empty())
    return NULL;
  
  my_sfd_event_t* result = &(my_sfd_record[my_sfd_first]);
  my_sfd_first = (my_sfd_first+1) & MY_SFD_RECORD_OFFSET_MASK;
  return result;
}

/*---------------------------------------------------------------------------*/

/* hijack the timerb interrupt */
interrupt (TIMERB1_VECTOR)
my_timerb_interrupt (void)
{  
  uint16_t clock_low  = TBR;
  uint16_t clock_high = my_timerb_count;
  int tbiv = TBIV;
  switch (tbiv) {
  case 0x02:  {
    /* TB2 - Zolertia SFD */
    uint8_t sfd_next = (my_sfd_last+1) & MY_SFD_RECORD_OFFSET_MASK;
    if (sfd_next != my_sfd_first) {
      my_sfd_record[my_sfd_last].is_up = CC2420_SFD_IS_1;
      my_sfd_record[my_sfd_last].event_time 
	= (((uint32_t)clock_high) << 16) + (uint32_t)clock_low;
      my_sfd_last = sfd_next;
    } else {
      MY_LED_ON(MY_G);
    }
    break;
  }
  case 0x0E: /* timer overflow */
    my_timerb_count ++;
    break;
  default:
    MY_LED_TOGGLE(MY_G);
  }
}

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
