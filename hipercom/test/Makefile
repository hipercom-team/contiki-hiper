#---------------------------------------------------------------------------
#            Examples of use of 'low-level.c' and performance tools
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

-include Makefile.target

ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = example-clock
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: ${CONTIKI_PROJECT} all-perf

all-perf: perf-send-ch25.${TARGET} perf-send-ch26.${TARGET} \
          perf-send-fast-ch25.${TARGET} perf-send-fast-ch26.${TARGET} \
          perf-recv-ch25.${TARGET} perf-recv-ch26.${TARGET}

CONTIKI=../../
include ${CONTIKI}/Makefile.include

#--------------------------------------------------

# can be overriden from command line
MOTEID=1
# can also be overriden
FIRMWARE=example-clock


dump:
	cd .. && python MoteManager.py --mote-id ${MOTEID} reset dump

flash: ${FIRMWARE}.${TARGET}
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
            --avoid-useless-reflash --firmware test/${FIRMWARE}.${TARGET} flash

flash-test: perf-send-ch26.${TARGET}
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
        --avoid-useless-reflash --firmware test/perf-send-ch26.${TARGET} flash

CH11MOTEID=8
flash-send-ch11: perf-send-ch11.${TARGET}
	cd .. && python MoteManager.py --mote-id ${CH11MOTEID} \
        --avoid-useless-reflash --firmware test/perf-send-ch11.${TARGET} flash

#---------------------------------------------------------------------------
