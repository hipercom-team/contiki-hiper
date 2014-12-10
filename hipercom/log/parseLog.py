

#---------------------------------------------------------------------------
#
# 2f = CC2420 prepare begin fifo
# 2F = CC2420 prepare end fifo
# 2t = CC2420 just before CCA
# 2C = CC2420 just after CCA
# 2o = CC2420 TX_OK
# 2S = SFD: 2S<node id><clock (4/net)><TAR>
# +  = same as 2o
# 2e = CC2420 not TX_OK
# -  = CC2420 TX_COLLISION (sometimes 2e before [no SFD], sometimes nothing)
# cr = retransmit (just before ctimer_set)
#
# 2I = CC2420 int RX 
# 2P = CC2420 reception process (begin process)
# 2R = CC2420 reception (end process)
# -> call stack
# rIN = RDC input
# rFR = RDC Frame Error
# ci = input RDC (rdc ok)
#
# S = send + packet seqno
# D = receive&decoded (before csma/ca) + packet seqno
#  (no D implies sometimes rFR)
#
#---------------------------------------------------------------------------

import struct, os, tempfile, warnings
import matplotlib, pylab
from pylab import *

def secToTar(x): return x*(1<<15)
def tarToSec(x): return x/float(1<<15)

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
    eventTable = {}
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


        expectedSizeTable = { '2t':4, 'S':4, '2P':4, '2F':4, '2f':4, '2S':9,
                              'cr':4 }
        shouldContinue = False
        for prefix, expectedSize in expectedSizeTable.iteritems():
            if packet.startswith(prefix) and len(packet) != expectedSize:
                warnings.warn("bad info size '%s'" % prefix)
                shouldContinue = True
        if shouldContinue: continue

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


        if (packet[0:2] in ["2I", "2o", "2t", "2C", "cr", "2e", "2f","2F",
                            "2P", "2R", "2S"] 
            or packet[0] in ['S','D']):
            if packet.startswith("2S"):
                tar = struct.unpack("<H", packet[7:9])[0]
            elif packet[0:3] not in ["rFR"]:
                tar = struct.unpack("<H", packet[2:4])[0]
            else: tar = struct.unpack("<H", packet[3:5])[0]
            if lastTar != None and tar < lastTar:
                clockHigh += (1<<16)
            fullTar = clockHigh + tar
            lastTar = tar
            lastTarPacket = packet
            
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

        def appendTable(table, key, value):
            if key not in table:
                table[key] = []
            table[key].append(value)

        if packet.startswith("2I"):
            rxList.append(fullTar)
            appendTable(eventTable,"int",tarToSec(fullTar))
            g.write("%s 1\n" % (fullTar/float(1<<15)))
            lastInterruptFullTar = fullTar
            #g.write("%s 1\n" % fullTar)

        if packet.startswith("cr"):
            rxList.append(fullTar)
            appendTable(eventTable,"retx",tarToSec(fullTar))
            g.write("%s 1.02\n" % (fullTar/float(1<<15)))
            #g.write("%s 1\n" % fullTar)

        if packet.startswith("2t"):
            appendTable(eventTable,"pre-cca",tarToSec(fullTar))
            g.write("%s 1.04\n" % (fullTar/float(1<<15)))
        if packet.startswith("2C"):
            appendTable(eventTable,"post-cca",tarToSec(fullTar))
            g.write("%s 1.06\n" % (fullTar/float(1<<15)))

        if packet.startswith("2o"):
            txList.append(fullTar)
            appendTable(eventTable,"txok",tarToSec(fullTar))
            g.write("%s 1.1\n" % (fullTar/float(1<<15)))

        #if packet.startswith("-"):
        #    appendTable(eventTable,"txerr",tarToSec(fullTar))

        if packet.startswith("rFR"):
            assert lastTarPacket.startswith("2R")
            appendTable(eventTable,"frerr",tarToSec(fullTar))

        if packet.startswith("rNU"):
            assert lastTarPacket.startswith("2R")
            appendTable(eventTable,"notus",tarToSec(fullTar))

        if packet.startswith("2f"):
            appendTable(eventTable,"fifostart",tarToSec(fullTar))

        if packet.startswith("2F"):
            appendTable(eventTable,"fifoend",tarToSec(fullTar))

        if packet.startswith("2P"):
            appendTable(eventTable,"rxproc",tarToSec(fullTar))

        if packet.startswith("2R"):
            appendTable(eventTable,"rxprocend",tarToSec(fullTar))

        # 2S = SFD: 2S<node id><clock (4/net)><TAR>
        if (packet.startswith("2S")):
            nodeId,syncClock = struct.unpack("!BI", packet[2:7])
            syncTar = struct.unpack("<H", packet[7:])[0]
            appendTable(eventTable,"sync",(syncClock, syncTar, 
            lastInterruptFullTar))
            #fullTar))
            print syncTar, fullTar


    g.close()

    #result = {"rxList":rxList, "txList":txList, "rxTable":rxTable,
    #          "txTable":txTable }
    return rxList, txList, rxTable, txTable, eventTable


def parseSnifferLog(fileName):
    result = ""
    f = open(fileName)
    while True:
        line = f.readline()
        if line == "": break
        data = eval(line.strip())
        if data[1] != "recv": continue
        result += data[2]
    f.close()

    s = result
    cmdList = []
    while len(s) > 0:
        pos = s.find("CA")
        if pos != 0: print "skipped sniffer stream %s"%pos
        s = s[pos+2:]
        cmdLen = ord(s[0])
        cmd = s[1:1+cmdLen]
        s = s[cmdLen+1:]
        cmdList.append(cmd)

    packetList = []
    lastSfd = None
    for cmd in cmdList:
        if cmd[0] == "p":
            (lost, rssi, linkQual, counter, t 
             )= struct.unpack("<BBBII", cmd[1:12])
            assert lost & 1 == 0 # only 32KiHz supported
            timestamp = t # 
            packetList.append((t,cmd[12:],rssi,linkQual,lastSfd))
            #print "---", t-lastSfd
            lastSfd = None
        elif cmd[0] == "s":
            isUp, padding, clock, serialClock = struct.unpack("<BBII", cmd[1:])
            info = ("sfd", isUp, clock)
            if isUp: lastSfd = clock
            else: 
                if lastSfd != None:
                    #print "DELTA SFD",lastSfd-clock
                    pass
        else: sys.stdout.write("?")

    MaxSfdDelay = secToTar(0.01)
    clockToFullTarSniffer = {}
    idAndClockList = []
    for clock, packet, rssi,linkQual, sfdClock in packetList:
        if sfdClock == None or clock-sfdClock > MaxSfdDelay or clock<sfdClock:
            print "** inconsistent sfd", clock, sfdClock
            sfdClock = clock
        if len(packet) == 13:
            (o1,o2,o3,revPanId,revDst,revSrc,senderClock
             )= struct.unpack("!BBBHHHI", packet) #== [0x41,0x88]
            clockToFullTarSniffer[senderClock] = clock #sfdClock #clock
        else:
            pos = packet.find("A"*50)
            if pos > 0:
                packetError = False
                try: packetId = int(packet[-70:].replace("A",""))
                except: packetError = True
                if packetError:
                    print "Packet parse error", repr(packet), rssi,linkQual
                    continue
                else: idAndClockList.append((packetId, sfdClock))
                
    return clockToFullTarSniffer, idAndClockList, packetList


offset = 0

version = 4
if version == 4:
    rootFileName = "EXP2_root.log"
    relayFileName = "EXP2_2.log"
    snifferFileName = "packet-EXP2.log"
    snifferOffset = secToTar(0.0008)
elif version == 3:
    rootFileName = "EXP1_root.log"
    relayFileName = "EXP1_2.log"
    snifferFileName = "packet-EXP1.log"
    relayOffset = secToTar(0.00025)
    snifferOffset = secToTar(0.0008)
    MaxPacketInterval = 0.1 #sec
    warnings.warn("overwritten later")
elif version == 1:
    rootFileName = "log_root.txt"
    relayFileName = "log_2.txt"
else:
    rootFileName = "root.log"
    relayFileName = "2.log"

(clockToFullTarSniffer, idAndClockList, packetSnifferList
) = parseSnifferLog(snifferFileName)

#packetInfoList = [
#    (sfdClock, rssi, len(packet))
#     for clock, packet, rssi, linkQual, sfdClock in packetSnifferList]

rxRoot, txRoot, rxPerId1,txPerId1, eventTable1 \
    = parseLog(rootFileName, skipCount=0*40)
print "-"*50
rxNode2, txNode2, rxPerId2,txPerId2, eventTable2 \
    = parseLog(relayFileName, offset, skipCount=0*15)

#---------------------------------------------------------------------------
# Compute median timeOffset for messages with rx/tx and same packetId

#print txPerId2, rxPerId2
#print txPerId1.keys()
#print rxPerId2.keys()

#commonIdSet = set(txPerId1.keys()).intersection(rxPerId2.keys())
#tarDiffList = sorted([ txPerId1[packetId] - rxPerId2[packetId]
#                       for packetId in commonIdSet ])
#timeOffset = tarDiffList[len(tarDiffList)/2]

# this timeOffset is overwritten with an actual sync. method
#print timeOffset

#---------------------------------------------------------------------------
# Find sequence of many packets sent successively

print "** Burst identification"

txList1 = txRoot

#print txList1
Window = 3
MaxPacketInterval = 0.6 # sec
firstBurstIdx = None
lastBurstIdx = None

for i in range(len(txList1)-Window):
    if txList1[i+Window] - txList1[i] <= secToTar(Window*MaxPacketInterval):
        lastBurstIdx = i
    else:
        firstBurstIdx = i+1
        lastBurstIdx = i+1

print "burst, first packet id=%s last=%s" % (firstBurstIdx, lastBurstIdx)
timeMin = txList1[firstBurstIdx]
timeMax = txList1[lastBurstIdx]
print "timeMin=%s timeMax=%s" % (tarToSec(timeMin), tarToSec(timeMax))


#---------------------------------------------------------------------------
# Synchronization based on beacon timestamps

def plot2(pairList, *args, **kwargs):
    (xList,yList) = zip(*pairList)
    plot(*((xList,yList)+args),**kwargs)

print "** Synchronization from beacons"

timeMargin = 3 # second

clockToFullTar1 = dict([
        (clock, fullTar) for clock,syncTar,fullTar in eventTable1["sync"]
        if fullTar > timeMin-secToTar(timeMargin)])

print "Beacons received by Root node: "
print "  "+" ".join(["sec#%s=%.4f" % (clock, tarToSec(clockToFullTar1[clock]))
                                 for clock in sorted(clockToFullTar1.keys())])
    

#..............................

clockToFullTar2 = dict([
        (clock, fullTar) for clock,syncTar,fullTar in eventTable2["sync"]
        if clock in clockToFullTar1])

print "Beacons received by node 2: "
print "  "+" ".join(["sec#%s=%.4f" % (clock, tarToSec(clockToFullTar2[clock]))
                                 for clock in sorted(clockToFullTar2.keys())])


timeOffsetList = [ (clockToFullTar1[clock]-clockToFullTar2[clock], 
                    tarToSec(clockToFullTar1[clock]))
                   for clock in sorted(clockToFullTar2.keys()) ]
#print timeOffsetList
node2Idx = 1
print "Synchronized Node 2 w.r.t Root(1) at",timeOffsetList[node2Idx][1]
timeOffset = timeOffsetList[node2Idx][0]

#..............................

# sniffer packet synchronization
timeOffsetSnifferList = [ 
    (clockToFullTar1[clock]-clockToFullTarSniffer[clock], 
     tarToSec(clockToFullTar1[clock]))
    for clock in sorted(clockToFullTarSniffer.keys())
    if (clock in clockToFullTar1)]

print "Beacons received by sniffer: "
print "  "+" ".join([
        "sec#%s=%.4f" % (clock, tarToSec(clockToFullTarSniffer[clock]))
        for clock in sorted(clockToFullTarSniffer.keys())
        if (clock in clockToFullTar1)])

print timeOffsetSnifferList

syncIdx = 1
print "Synchronized Sniffer w.r.t Root(1) at",timeOffsetSnifferList[syncIdx][1]
timeOffsetSniffer = timeOffsetSnifferList[syncIdx][0]


#relayOffset,snifferOffset = 0,0
#timeOffset+=relayOffset 
#timeOffsetSniffer-=snifferOffset

print "Time offset between node 2 and root(1):", tarToSec(timeOffset)
print "Time offset between sniffer and root(1):", tarToSec(timeOffsetSniffer)

#exitNow

#---------------------------------------------------------------------------

yOfEvent = {
    'int' : 0.1, 
    'rxproc': 0.13,
    'rxprocend': 0.16,
    'frerr': 0.2,
    'notus': 0.23,
    
    'fifostart': 0.3,
    'fifoend': 0.33,
    'pre-cca': 0.4, 
    'post-cca': 0.43, 
    'txok': 0.5,
    'txerr': 0.55,
    'retx': 0.6
}


#timeAdjust  = +( 15.3587-15.3525 )
timeAdjust = 0

clockList1 = []
eventCodeList1 = []
for eventName, timeList in eventTable1.iteritems():
    if eventName not in yOfEvent: continue
    clockList1 += timeList
    eventCodeList1 += [ yOfEvent[eventName] ] * len(timeList)

clockList2 = []
eventCodeList2 = []
for eventName, timeList in eventTable2.iteritems():
    if eventName not in yOfEvent: continue
    clockList2 += [x+tarToSec(timeOffset)+timeAdjust for x in timeList ]
    eventCodeList2 += [ yOfEvent[eventName] ] * len(timeList)

clockPacketSniffer = [(tarToSec(fullTar+timeOffsetSniffer),0.05)
                      for packetId,fullTar in idAndClockList]

rssiList = [
    rssi for clock, packet, rssi, linkQual, sfdClock in packetSnifferList]

minRssi = min(rssiList)
rssiDelta = float( max(max(rssiList)-min(rssiList), 10) )

packetPlotList = [
    (tarToSec(sfdClock+timeOffsetSniffer), (rssi-minRssi)/rssiDelta * 0.04,
     tarToSec(clock+timeOffsetSniffer), packet)
     for clock, packet, rssi, linkQual, sfdClock in packetSnifferList]

#for clock, packet, rssi, linkQual, sfdClock in packetSnifferList:
#    print tarToSec(sfdClock+timeOffsetSniffer) #, repr(packet), rssi, linkQual

#packetPlotList = [
#    (tarToSec(clock+timeOffsetSniffer),0.04 * (rssi/256.0))
#    for clock, rssi, packetLen in packetPlotList]

#print packetPlotList

plot(clockList1, eventCodeList1, '+')
plot(clockList2, eventCodeList2, 'rx')
plot2(clockPacketSniffer, 'g+')

for i in range(len(packetPlotList)):
    sfdClock,rssi,intClock, packet = packetPlotList[i]
    if len(packet) == 0: c = (1.0,0,0,0)
    else: c = (0,0,0,0)
    plot([sfdClock, intClock], [rssi]*2, '-', color=c)
#plot2(packetPlotList, '+', color=(0,0,0,0))
#plot(pointList1, "+")

axis([tarToSec(timeMin),tarToSec(timeMax),0,5])
grid(True)
xlabel("time (sec)")
show()

#---------------------------------------------------------------------------

o = 0.5

if False:
    runGnuplot("""
set grid
plot 'packet-sent.dat' w l, \
  'packet-recv.dat' using ($1+%s):($2) w l, \
  'packet-retx.dat' using ($1+%s):($2) w l
#plot 'correlation.dat' 
plot [%s:%s] [0:2.5] 'root.log.dat', '2.log.dat' using ($1+%s):($2) 

pause -1
""" % (tarToSec(timeOffset), tarToSec(timeOffset),
       #tarToSec(timeMin)+o, 
       #tarToSec(timeMin)+o+0.5, 
       tarToSec(timeMin), tarToSec(timeMax), 
       tarToSec(timeOffset)))

#---------------------------------------------------------------------------
