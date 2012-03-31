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

/* See http://www2.ece.ohio-state.edu/~bibyk/ee582/telosMote.pdf
 */

/*---------------------------------------------------------------------------*/

#define MY_LEDS_DIR      P5DIR
#define MY_LEDS_SEL      P5SEL
#define MY_LEDS_OUT      P5OUT

#define MY_LED_ON(x)     (MY_LEDS_OUT &= ~(x))
#define MY_LED_OFF(x)    (MY_LEDS_OUT |= (x))
#define MY_LED_TOGGLE(x) (MY_LEDS_OUT ^= (x))

#define MY_R        0x10
#define MY_G        0x20
#define MY_B        0x40

/*---------------------------------------------------------------------------*/

void my_timerb_init(uint8_t use8Mhz)
{
}
