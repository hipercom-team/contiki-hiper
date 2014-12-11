#---------------------------------------------------------------------------
# Packet Sniffer
# Author: Cedric Adjih (Inria)
#---------------------------------------------------------------------------
# Copyright (c) 2012-2014, Inria
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
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

MOTEID?=7 # can be overriden from command line

MOTECTL=python moteSniffer.py --mote-id ${MOTEID}

sniff: ; ${MOTECTL} sniffer-wireshark

sniff-fast: ; ${MOTECTL} --high-speed sniffer-wireshark

record: ; ${MOTECTL} --record packet-DATE.log --high-speed sniffer-wireshark 

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

html-doc:
	for i in *.md ; do grip --export $$i $$(basename $$i .md).html ; done

#---------------------------------------------------------------------------

