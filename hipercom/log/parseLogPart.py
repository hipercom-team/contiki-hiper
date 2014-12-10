
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





#timeMin = min(txPerId1.values())
#timeMax = max(txPerId1.values())

f = open("packet-sent.dat", "w") 
for i,clock in enumerate(sorted(txPerId1.values())):
    f.write("%s %s\n" % (tarToSec(clock), i))
f.close()

f = open("packet-recv.dat", "w") 
for i,clock in enumerate(sorted(rxPerId2.values())):
    f.write("%s %s\n" % (tarToSec(clock), i))
f.close()

f = open("packet-retx.dat", "w") 
for i,clock in enumerate(sorted(txPerId2.values())):
    f.write("%s %s\n" % (tarToSec(clock), i))
f.close()
