#---------------------------------------------------------------------------
#                           Packet Sniffer
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

import os, re

#---------------------------------------------------------------------------

FreqStr = "32Khz"
#FreqStr = "8Mhz"

SubDir = "Exp-"+FreqStr
if FreqStr == "32Khz": freq = 32*1024
else: freq = 244*32768

rLogName = re.compile("packet-([0-9]+)[.]log")

fileTable = {}

for fileName in os.listdir(SubDir):
    m = rLogName.match(fileName)
    if m != None:
        nodeId = int(m.group(1))
        fileTable[nodeId] = fileName

#---------------------------------------------------------------------------

packetTable = {}

packetList = []

isFirst = True
for nodeId,fileName in fileTable.iteritems():
    f = open(SubDir+"/"+fileName)
    lastSfdUp = None
    offset = 0
    while True:
        try:
            line = f.readline()
            if line == "": break
            info = eval(line.strip())
        except: break
        if info[0] == "sfd":
            newSfd = info[2] + offset
            if (lastSfdUp != None and newSfd < lastSfdUp
                and lastSfdUp - newSfd > (1L << 31)):
                offset += (1L<<32)
                newSfd = info[2] + offset
                print "***", offset
            if info[1] == 1:
                lastSfdUp = info[2]+offset
        elif info[0] == "packet":
            packetId = info[1]
            if packetId not in packetTable:
                packetTable[packetId] = {}
            #if lastSfdUp < 0: print info
            if nodeId in packetTable[packetId]: 
                print info
                #continue
            assert nodeId not in packetTable[packetId]
            packetTable[packetId][nodeId] = lastSfdUp
            if isFirst: packetList.append(packetId)
            #lastSfdUp = None
        else: raise RuntimeError("unknown")
    f.close()
    #print offset
    isFirst = False


nodeList = sorted(fileTable.keys())
nbNode = len(nodeList)
allClockList = []
baseClockList = None
for packetId in packetList:
    clockTable = packetTable[packetId]
    if len(clockTable) == nbNode:
        clockList = [clockTable[nodeId] for nodeId in nodeList]
        if baseClockList == None:
            baseClockList = clockList
            print "baseClock:", baseClockList
        clockList = [ clockList[i] - baseClockList[i] for i in range(nbNode) ]
        allClockList.append(clockList)
        #if clockList[0] < 0: print clockList

data = { 'nodeList': nodeList, 'nbNode': nbNode, 'allClockList': allClockList }

f = open("allclock.pydata", "w")
f.write(repr(data)+"\n")
f.close()

#---------------------------------------------------------------------------

f = open("packet-time.log", "w")
last = 0
for clockList in allClockList:
    r = " ".join(["%s" % repr(x) for x in clockList])
    f.write(r + "\n")
    if clockList[0] < last:
        print clockList[0], last
    last = clockList[0]
f.close()
finalClockList = clockList

TemplateCmd = """
set grid
set term post eps color
set output '%s.ps'
plot """

rawClock = ",".join(["'packet-time.log' u ($1/FREQ):($%d/FREQ) w dots" %(i+1) 
                     for i in range(1, nbNode)])

diffClock = ",".join([
        "'packet-time.log' u ($1/FREQ):(($%d-$1)/FREQ) w dots" %(i+1)
        for i in range(1, nbNode)])


# t_i = (1+coef_i) * t_0

print finalClockList
exitNow

prec = (1L << 64L)
#prec = 1.0
coef = [ ((finalClockList[i]-finalClockList[0])*prec) // finalClockList[0] for i in range(nbNode) ]

coef = [ float(x)/float(prec) for x in coef ]
#print coef

correctedClock = ",".join([
        "'packet-time.log' u ($1/FREQ):((($%d-$1) - %lf * $1)/FREQ) w dots" 
        %(i+1, coef[i]) for i in range(1, nbNode)])

#diffClock = ",".join([
#        "'packet-time.log' u 1:($%d-$1) w dots" %(
#            i+1, clockList[i]/clockList[1]) 
#        for i in range(1, nbNode)])

f = open("gen-packet-time.plot", "w")
f.write(TemplateCmd % "raw-time" + rawClock.replace("FREQ",repr(freq)) + "\n")
f.write(TemplateCmd % "diff-time" 
        + diffClock.replace("FREQ",repr(freq)) + "\n")
f.write(TemplateCmd % "corrected-time" 
        + correctedClock.replace("FREQ",repr(freq)) + "\n")
f.close()

os.system("gnuplot gen-packet-time.plot")

#---------------------------------------------------------------------------

# plot 'packet-time.log' w dots, 'packet-time.log' u 1:3 w dots, 'packet-time.log' u 1:4 w dots

#---------------------------------------------------------------------------
