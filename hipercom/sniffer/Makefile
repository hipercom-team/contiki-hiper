#---------------------------------------------------------------------------
#                           Packet Sniffer
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

-include Makefile.target

ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = sniffer
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: ${CONTIKI_PROJECT}

CONTIKI=../../
include ${CONTIKI}/Makefile.include

#--------------------------------------------------

MOTEID?=6 # can be overriden from command line

MOTECTL=python moteSniffer.py --mote-id ${MOTEID}

sniff: ; ${MOTECTL} sniffer-wireshark

sniff-fast: ; ${MOTECTL} --high-speed sniffer-wireshark

dump: ; ${MOTECTL} sniffer-text

opera: ; ${MOTECTL} sniffer-opera

short: ; ${MOTECTL} --short --log packet-${MOTEID}.log sniffer-text

rssi: ; ${MOTECTL} rssi

rssi-fast: ; ${MOTECTL} --high-speed rssi

rssi-dac: ; ${MOTECTL} rssi-dac

rssi-ch25: ; ${MOTECTL} --channel 25 rssi

flash: sniffer.${TARGET}
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
            --avoid-useless-reflash --firmware sniffer/sniffer.${TARGET} flash

flash-rssi: rssi-sniffer.${TARGET}
	cd .. && python MoteManager.py --mote-id ${MOTEID} \
         --avoid-useless-reflash --firmware sniffer/rssi-sniffer.${TARGET} flash

#---------------------------------------------------------------------------

save:
	(printf "TARGET=${TARGET}\nMOTEID=${MOTEID}\n" > Makefile.target)

#---------------------------------------------------------------------------
