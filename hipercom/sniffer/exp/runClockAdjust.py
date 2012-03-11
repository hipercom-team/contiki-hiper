#---------------------------------------------------------------------------
#                         Clock Synchronization
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

f = open("allclock.pydata")
data = eval(f.read().strip())
f.close()

freq = 8000000
freq = 32768

#---------------------------------------------------------------------------

nodeList = data["nodeList"]
nbNode = data["nbNode"]
allClockList = data["allClockList"]

#---------------------------------------------------------------------------

last = 0
for clock in allClockList:
    if clock[0] < last: print clock[0], last
    last = clock[0]

finalClockList = allClockList[-1]
#prec = (1L << 64L)
#prec = 1.0
coef = [ float(finalClockList[i]) / float(finalClockList[0])
         for i in range(nbNode) ]

for syncIntervalSec in [5, 25, 100]:
    syncInterval = syncIntervalSec * (freq)
    
    f = open("time-sync%s.dat" % syncIntervalSec, "w")
    c = coef[:]
    lastSyncClock = allClockList[0]
    for clockList in allClockList:
        if clockList[0]-lastSyncClock[0] > syncInterval:
            c = [ float(lastSyncClock[i] - clockList[i])
                  / float(lastSyncClock[0] - clockList[0])
                  for i in range(nbNode) ]
            lastSyncClock = clockList[:]
            print lastSyncClock, "*", syncInterval
        thClock = [ (clockList[0]-lastSyncClock[0]) * c[i] + lastSyncClock[i]
                    for i in range(nbNode) ]
        clock = [float(clockList[0])] + [ thClock[i] - clockList[i] 
                                          for i in range(nbNode) ]
        clock = [ x for x in clock ]
        f.write(" ".join( ["%s" % x for x in clock]) + "\n")
    f.close()

#---------------------------------------------------------------------------
