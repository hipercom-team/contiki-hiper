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
MOTEID=10
# can also be overriden
FIRMWARE=example-ctimer

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


myclean:
	make clean TARGET=sky
	make clean TARGET=z1
	rm -f *.sky

#---------------------------------------------------------------------------

test-contiki-clock.S: test-contiki-clock.c
	msp430-gcc -DPROJECT_CONF_H=\"project-conf.h\" -DCONTIKI=1 -DCONTIKI_TARGET_SKY=1 -Os -fno-strict-aliasing -ffunction-sections -Wall -mmcu=msp430x1611 -g   -I. -I../..//platform/sky/. -I../..//platform/sky/dev -I../..//platform/sky/apps -I../..//platform/sky/net -I../..//cpu/msp430/f1xxx -I../..//cpu/msp430/. -I../..//cpu/msp430/dev -I../..//core/dev -I../..//core/lib -I../..//core/net -I../..//core/net/mac -I../..//core/net/rime -I../..//core/net/rpl -I../..//core/sys -I../..//core/cfs -I../..//core/ctk -I../..//core/lib/ctk -I../..//core/loader -I../..//core/. -I../..//platform/sky/ -DCONTIKI_VERSION_STRING=\"Contiki-2.5-release-305-g153864b\" -DAUTOSTART_ENABLE -c test-contiki-clock.c -S -o test-contiki-clock.S

#---------------------------------------------------------------------------