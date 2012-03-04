/*---------------------------------------------------------------------------
 *                     Sniffer Project Configuration
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/* -- Change these only if you know what you are doing: */

/* We will manually set the TimerB, so no CONF_SFD_TIMESTAMPS */
#define CONF_SFD_TIMESTAMPS 0

/* Defining this will comment out the function
   cc24240_timerb1_interrupt in cc2420-arch-sfd.c */
#define HIJACK_TIMERB1_INTERRUPT

/*---------------------------------------------------------------------------*/
