#---------------------------------------------------------------------------
# Mote Management
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

import sys, time, select, os
sys.path.append("..")
sys.path.append(".")
os.chdir("..")
from MoteManager import MoteManager

#---------------------------------------------------------------------------

manager = MoteManager()
moteTable = manager.getMoteIdTable()

mote = manager.getMoteById(int(sys.argv[1]))
#mote.flashFirmware("test-serial.z1")
mote.reset()
#mote.openPort(speed=2000000)
mote.openPort(speed=115200)

if False:
    timeLast = time.time()
    count = 0

    while True:
        c = mote.port.read(1)
        #if c != len(c)*'A': 
        #    sys.stdout.write(c) 
        #    sys.stdout.flush()
        count += len(c)
        #sys.stdout.write(c)
        #sys.stdout.flush()
        clock = time.time()
        if clock - timeLast > 1:
            print ("dt=%s  %s byte/sec  %s bps" 
                   % (clock-timeLast, count/float (clock-timeLast), 
                      count*8 /float (clock-timeLast)))
            timeLast = clock
            count = 0


def hasInput():
    rlist, wlist, xlist = select.select([sys.stdin.fileno()], [], [], 0)
    return len(rlist) > 0

lastSend = time.time()

while True:
    data = ""
    while mote.port.inWaiting():
        data += mote.port.read(1)
    if len(data) > 0:
        print time.time(), repr(data)
    if time.time() - lastSend > 0.1:
        mote.port.write("A")
        lastSend = time.time()

#---------------------------------------------------------------------------
        
while True:
    data = ""
    while mote.port.inWaiting():
        data += mote.port.read(1)
    if len(data) > 0:
        print repr(data)

    data = ""
    while hasInput():
        data += sys.stdin.read(1)
    if len(data) > 0:
        data = data.strip()
        cmd = "CI"+chr(len(data))+data
        print "->", repr(cmd)
        mote.port.write(cmd)

while True:
    while mote.port.read(1) != 'C':
        pass
    if mote.port.read(1) != 'A':
        continue
    if hasInput():
        mote.port.write("G")
        while hasInput():
            sys.stdin.read(1)
    blockSize = ord(mote.port.read(1))
    data = mote.port.read(blockSize)
    print repr(data)

#---------------------------------------------------------------------------
