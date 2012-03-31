#---------------------------------------------------------------------------
#                            Mote Management
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
# Copyright 2012 Inria. All rights reserved. Distributed only with permission.
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
