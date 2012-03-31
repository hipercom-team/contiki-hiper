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

void my_uart0_init(void)
{
  ME2 &= ~USPIE1;              /* disable USART1 SPI mode */
  ME2 &= ~UTXIE1;              /* Disable TX interrupt */
  ME2 &= ~URXIE1;              /* Disable RX interrupt */
  ME2 |= UTXE1 + URXE1;        /* Enable Tx/Rx - XXX not in low-level-z1.c */
  IFG2 &= ~URXIFG1 ;           /* Clear pending interrupts */
  IFG2 &= ~UTXIFG1;
}

void my_uart0_init_2Mbps()
{
  U1BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver! */
  my_uart0_init();
}

void my_uart0_switch_to_2Mbps()
{
  U1BR0 = 0x5; /* 8M/1600000 = 5 -> 1.6 Mbps but ok for 2 Mbps on receiver */
}

#define MY_IS_UART0_BUSY ((IFG2 & UTXIFG1) == 0)

static inline void my_uart0_write(uint8_t data)
{
  while(MY_IS_UART0_BUSY);
  TXBUF1 = data;
}

static inline uint8_t my_uart0_read(void)
{ return U1RXBUF; }

static inline uint8_t my_uart0_has_data(void)
{ return IFG2 & URXIFG1; }

/*---------------------------------------------------------------------------*/
