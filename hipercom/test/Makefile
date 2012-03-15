#---------------------------------------------------------------------------
#            Examples of use of 'low-level.c' and performance tools
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = example-clock
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: ${CONTIKI_PROJECT} all-perf

all-perf: perf-send-ch25.z1 perf-send-ch26.z1 \
          perf-send-fast-ch25.z1 perf-send-fast-ch26.z1 \
          perf-recv-ch25.z1 perf-recv-ch26.z1

CONTIKI=../../
include ${CONTIKI}/Makefile.include

#--------------------------------------------------

MOTEID=1 # can be overriden from command line
FIRMWARE=example-clock # can also be overriden

dump:
	cd .. && python MoteManager.py --mote-id ${MOTEID} reset dump

flash: ${FIRMWARE}.z1
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
            --avoid-useless-reflash --firmware test/${FIRMWARE}.z1 flash

flash-test: perf-send-ch26.z1
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
            --avoid-useless-reflash --firmware test/perf-send-ch26.z1 flash

CH11MOTEID=8
flash-send-ch11: perf-send-ch11.z1
	cd .. && python MoteManager.py --mote-id ${CH11MOTEID} \
            --avoid-useless-reflash --firmware test/perf-send-ch11.z1 flash

#---------------------------------------------------------------------------
