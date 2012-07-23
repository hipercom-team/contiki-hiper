


# 2f = CC2420 prepare begin fifo
# 2F = CC2420 prepare end fifo
# 2t = CC2420 just before CCA
# 2C = CC2420 just after CCA
# 2o = CC2420 TX_OK
# +  = same as 2o
# 2e = CC2420 not TX_OK
# -  = CC2420 TX_COLLISION (sometimes 2e before [no SFD], sometimes nothing
# cr = retransmit (just before ctimer_set)

# 2I = CC2420 int RX 
# 2P = CC2420 reception process (begin process)
# 2R = CC2420 reception (end process)
# -> call stack
# rIN = RDC input
# rFR = RDC Frame Error
# ci = input RDC (rdc ok)


import struct, os, tempfile

#import scipy, numpy
#import scipy.signal.signaltools

def runGnuplot(commandText):
    tmpFileName = tempfile.mktemp()
    f = open(tmpFileName, "w")
    f.write(commandText)
    f.close()
    os.system("gnuplot "+tmpFileName)

MaxTarDiff = 2000

def parseLog(fileName, timeOffset = 0, skipCount=0):
    rxList = []
    txList = []
    rxTable = {}
    txTable = {}
    f = open(fileName)
    g = open(fileName+".dat", "w")
    baseClock = None
    tarDiff = None
    lastTar = None

    clockHigh = 0
    lastPacketId = None
    offsetPacketId = 0

    while True:
        line = f.readline()
        if line == "": break
        if skipCount >0:
            skipCount -= 1
            continue
        clock, packet = eval(line.strip())

        if baseClock == None: 
            baseClock = clock
            
        if 0 and (packet[0:2] in ["2I", "2o"] or packet[0] in ['S','D']):
            tar = struct.unpack("<H", packet[2:4])[0]

            relClock = long( (clock+timeOffset-baseClock)* (1L<<15L)) # 'tar'PC
            if tarDiff == None:
                tarDiff = (relClock-tar) % (1L<<16)
            ##print "TARDIFF", tarDiff, (relClock-tar-tarDiff) % (1L<<16)

            deltaTar = (relClock-tarDiff-tar) % (1L<<16)
            assert (deltaTar < MaxTarDiff or deltaTar > (1<<16)-MaxTarDiff)
            ##print deltaTar

            fullTar = (((relClock - tarDiff) >> 16) << 16) | tar
            ##if deltaTar < (1<<15): fullTar += (1<<16)
            #print relClock, (relClock >>16)<<16, (((relClock-tarDiff) >> 16) << 16), tar, fullTar, deltaTar
            ##print deltaTar
            ##print fullTar, lastTar
            if lastTar != None:
                assert fullTar >= lastTar
            lastTar = fullTar

        if packet[0:2] in ["2I", "2o"] or packet[0] in ['S','D']:
            tar = struct.unpack("<H", packet[2:4])[0]
            if lastTar != None and tar < lastTar:
                clockHigh += (1<<16)
            fullTar = clockHigh + tar
            lastTar = tar
            
        if packet[0] == 'S' or packet[0] == 'D':
            packetId = struct.unpack("!B", packet[1])[0]
            if lastPacketId == None:
                lastPacketId = packetId
            #elif packetId < lastPacketId-(1<<7): # wrap-around
            #    offsetPacketId += (1<<8)
            lastPacketId = packetId
            fullPacketId = offsetPacketId + packetId
            #print fullPacketId, packetId
            if packet[0] == "S":
                txTable[fullPacketId] = fullTar
            elif packet[0] == "D":
                rxTable[fullPacketId] = fullTar
            else: raise ValueError("code", packet[0])

        if packet.startswith("2I"):
            rxList.append(fullTar)
            g.write("%s 1\n" % (fullTar/float(1<<15)))
            #g.write("%s 1\n" % fullTar)

        if packet.startswith("2o"):
            txList.append(fullTar)
            g.write("%s 2\n" % (fullTar/float(1<<15)))
    g.close()

    return rxList, txList, rxTable, txTable


offset = 2

rxRoot, txRoot, rxPerId1,txPerId1 \
    = parseLog("log_root.txt", skipCount=0*40)
print "-"*50
rxNode2, txNode2, rxPerId2,txPerId2 \
    = parseLog("log_2.txt", offset, skipCount=0*15)

#---------------------------------------------------------------------------

commonIdSet = set(txPerId1.keys()).intersection(rxPerId2.keys())
tarDiffList = sorted([ txPerId1[packetId] - rxPerId2[packetId]
                       for packetId in commonIdSet ])
timeOffset = tarDiffList[len(tarDiffList)/2]

#---------------------------------------------------------------------------

runGnuplot("""

#plot 'correlation.dat' 
plot [:] [0:2.5] 'log_root.txt.dat', 'log_2.txt.dat' using ($1-%s):(3.1-$2)

pause -1
""" % (-timeOffset/32768.0))
