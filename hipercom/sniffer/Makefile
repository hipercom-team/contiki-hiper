#---------------------------------------------------------------------------
#                           Packet Sniffer
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = sniffer
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: ${CONTIKI_PROJECT}

CONTIKI=../../
include ${CONTIKI}/Makefile.include

#--------------------------------------------------

SNIFFER_MOTEID=1 # can be overriden from command line

MOTECTL=python moteSniffer.py --reset --mote-id ${SNIFFER_MOTEID}

sniff: ; ${MOTECTL} sniffer-wireshark

sniff-fast: ; ${MOTECTL} --high-speed sniffer-wireshark

dump: ; ${MOTECTL} sniffer-text

rssi: ; ${MOTECTL} rssi

rssi-fast: ; ${MOTECTL} --high-speed rssi

rssi-dac: ; ${MOTECTL} rssi-dac

flash: sniffer.z1
	cd .. && python MoteManager.py --mote-id ${SNIFFER_MOTEID} \
            --avoid-useless-reflash --firmware sniffer/sniffer.z1 flash

#---------------------------------------------------------------------------
