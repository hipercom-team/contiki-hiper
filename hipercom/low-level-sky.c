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

/* See: ./cpu/msp430/f1xxx/uart1.c 
   and http://www.ti.com/lit/ug/slau049f/slau049f.pdf */

/*---------------------------------------------------------------------------*/

/* WARNING: this is actually UART1 */

void my_uart_init(void)
{
  ME2 &= ~USPIE1;              /* Disable USART1 SPI mode */
  ME2 |= UTXE1 + URXE1;        /* Enable Tx/Rx -XXX do also in low-level-z1.c */
  IE2 &= ~UTXIE1;              /* Disable TX interrupt */
  IE2 &= ~URXIE1;              /* Disable RX interrupt */
  IFG2 &= ~URXIFG1 ;           /* Clear pending interrupts */
  IFG2 &= ~UTXIFG1;
  U1TCTL &= ~URXSE; /* Erroneous char won't set URXIFG1 -XXX do also in -z1.c */

  DMACTL0 = DMA0TSEL_0; /* no DMA (slau049f.pdf: "DMAREQ bit (software trigger)"
   XXX maybe undo more of cpu/msp430/f1xxx/uart1.c uart1_init initialisation */
}

/* XXX: these are not implemented properly yet for this platform */
#warning "my_uart_init_2Mbps / my_uart_switch_to_2Mbps will NOT work properly"
void my_uart_init_2Mbps()
{
  U1BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver! */
  my_uart_init();
}

void my_uart_switch_to_2Mbps()
{
  U1BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver */
}
/* --- */

#define MY_IS_UART_BUSY ((IFG2 & UTXIFG1) == 0)

static inline void my_uart_write(uint8_t data)
{
  while(MY_IS_UART_BUSY);
  TXBUF1 = data;
}

static inline uint8_t my_uart_read(void)
{ return U1RXBUF; }

static inline uint8_t my_uart_has_data(void)
{ return IFG2 & URXIFG1; }

/*---------------------------------------------------------------------------*/
