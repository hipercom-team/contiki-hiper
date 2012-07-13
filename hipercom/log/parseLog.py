


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

import scipy, numpy
import scipy.signal.signaltools

def runGnuplot(commandText):
    tmpFileName = tempfile.mktemp()
    f = open(tmpFileName, "w")
    f.write(commandText)
    f.close()
    os.system("gnuplot "+tmpFileName)

MaxTarDiff = 2000

def parseLog(fileName, timeOffset = 0):
    rxList = []
    txList = []
    f = open(fileName)
    g = open(fileName+".dat", "w")
    baseClock = None
    tarDiff = None
    lastTar = None

    while True:
        line = f.readline()
        if line == "": break
        clock, packet = eval(line.strip())


        if baseClock == None: 
            baseClock = clock
            
        if packet[0:2] in ["2I", "2o"]:
            tar = struct.unpack("<H", packet[2:4])[0]

            relClock = long( (clock+timeOffset-baseClock)* (1L<<15L))
            if tarDiff == None:
                tarDiff = (relClock-tar) % (1L<<16)
            #print tarDiff, (relClock-tar-tarDiff) % (1L<<16)
            deltaTar = (relClock-tarDiff - tar) % (1L<<16)
            assert (deltaTar < MaxTarDiff or deltaTar > (1<<16)-MaxTarDiff)
            #print deltaTar
            
            fullTar = (((relClock-tarDiff) >> 16) << 16) | tar
            #print deltaTar
            #if deltaTar > (1<<15): fullTar += (1<<16)
            if lastTar != None:
                assert fullTar >= lastTar
            lastTar = fullTar

        if packet.startswith("2I"):
            rxList.append(fullTar)
            g.write("%s 1\n" % (fullTar/float(1<<15)))
            #g.write("%s 1\n" % fullTar)

        if packet.startswith("2o"):
            txList.append(fullTar)
            g.write("%s 2\n" % (fullTar/float(1<<15)))
    g.close()

    return rxList, txList


offset = 2

rxRoot, txRoot = parseLog("log_root.txt")
print "-"*50
rxNode2, txNode2 = parseLog("log_2.txt", offset)

# 
if 0:
    s1 = txRoot[len(txRoot)/2]
    s2 = rxNode2[len(rxNode2)/2]
    
    d = (1<<15) * 2
    
    data1 = [x-s1+d for x in txRoot if s1-d < x < s1+d ]
    data2 = [x-s2+d for x in rxNode2 if s2-d < x < s2+d ]


# http://stackoverflow.com/questions/6157791/find-phase-difference-between-two-inharmonic-waves

if 0:
    ts = max(max(data1), max(data2))+1
    ts = 1019
    rootArray = scipy.zeros(ts)  #array([(t in txRoot) for t in range(ts)])
    node2Array = scipy.zeros(ts) #array([(t in rxNode2) for t in range(ts)])

    #fil = [1, 3, 5, 3, 1]
    fil,k = [1, 3, 5, 9, 5, 3, 1],3
    for t in txRoot:
        for j in range(len(fil)):
            rootArray[(t+j-k) % ts] += fil[j]
    for t in rxNode2:
        for j in range(len(fil)):
            node2Array[(t+j-k) % ts] += fil[j]


data1 = txRoot[:]
data2 = rxNode2[:]
bin = 128

ts = max(max(data1),max(data2))//bin+1
print ts

vector1 = scipy.zeros(ts)  #array([(t in txRoot) for t in range(ts)])
vector2 = scipy.zeros(ts) #array([(t in rxNode2) for t in range(ts)])

for x in data1:
    vector1[x//bin] += 1
for x in data2:
    vector2[x//bin] += 1

#xcorr = list(scipy.signal.fftconvolve(vector1, vector2))
xcorr = scipy.signal.signaltools.correlate(vector1, vector2)

dt = numpy.arange(1-ts, ts)
time_shift = dt[xcorr.argmax()]
print time_shift

f = open("correlation.dat", "w")
for i in range(len(xcorr)):
    f.write("%d %d\n" % (i, xcorr[i]))
f.close()

runGnuplot("""

#plot 'correlation.dat' 
plot [40:] [0:2.5] 'log_root.txt.dat', 'log_2.txt.dat' using ($1-%s):(3.1-$2)

pause -1
""" % ((-time_shift*bin)/float(1<<15) ))
