/*---------------------------------------------------------------------------
 *                         Low Level Tools
 *        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
 *                       Copyright 2012 Inria. 
 *            All rights reserved. Distributed only with permission.
 *---------------------------------------------------------------------------*/

typedef uint32_t my_time_t;

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

/*---------------------------------------------------------------------------
 * Platform specific files
 *---------------------------------------------------------------------------*/

#ifdef CONTIKI_TARGET_Z1
#include "low-level-z1.c"
#endif /* CONTIKI_TARGET_Z1 */

#ifdef CONTIKI_TARGET_SKY
#include "low-level-sky.c"
#endif /* CONTIKI_TARGET_SKY */

/*---------------------------------------------------------------------------*/
