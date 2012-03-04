#---------------------------------------------------------------------------
#                  Examples of use of 'low-level.c'
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = example-clock
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: ${CONTIKI_PROJECT}

CONTIKI=../../
include ${CONTIKI}/Makefile.include

#--------------------------------------------------

MOTEID=2 # can be overriden from command line

dump:
	cd .. && python MoteManager.py --mote-id ${MOTEID} reset dump

flash: example-clock.z1
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
            --avoid-useless-reflash --firmware test/example-clock.z1 flash

#---------------------------------------------------------------------------
