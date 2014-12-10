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

import sys, time, select, optparse, random, struct, hashlib
import datetime, os
import socket
import threading, curses, atexit # for bars

sys.path.append("..")
import MoteManager

#---------------------------------------------------------------------------

LogVersion = (1,0)

#--------------------------------------------------

def makeCmd(cmd):
    return "CI"+chr(len(cmd))+cmd

ResetCmd = makeCmd("")

def makeSyncCmd():
    nonce = "".join([chr((random.randint(1,255)+ord('C')) & 0xff) # no 'C'
                     for i in range(8)])
    return makeCmd("!"+nonce)
    
#--------------------------------------------------

dbgSync = False

def syncMote(mote, timeLimit = None):
    global dbgSync
    mote.write(ResetCmd)
    cmdInvoke = makeSyncCmd()
    cmdAnswer = "CA"+cmdInvoke[2:]
    mote.write(cmdInvoke)

    startTime = time.time()
    result = ""
    #print repr(cmdAnswer)
    while ((timeLimit == None or time.time()-startTime < timeLimit)
           and len(result) != len(cmdAnswer)):
        if not mote.port.inWaiting():
            time.sleep(0.01)
            continue
        oneChar = mote.port.read(1)
        if dbgSync:
            sys.stdout.write(repr(oneChar))
        result +=  oneChar
        if not cmdAnswer.startswith(result):
            result = ""
        #sys.stdout.write(repr(result))
        #sys.stdout.flush()

    return result == cmdAnswer

def attemptSyncMote(mote, timeLimit, attemptLimit):
    for i in range(attemptLimit):
        if syncMote(mote, timeLimit): return True
        sys.stdout.write(".")
        sys.stdout.flush()
    return False

def moteFastRead(mote):
    if not mote.port.inWaiting():
        time.sleep(0.01)
        if not mote.port.inWaiting():
            return ""
    return mote.port.read()


def moteGetCmdAnswer(mote):
    while True:
        while mote.port.read(1) != 'C':
            continue
        if mote.port.read(1) != 'A':
            continue
        lenChar = mote.port.read(1)
        #return 'CA'+lenChar+
        return mote.port.read(ord(lenChar))

def dumpMote(mote):
    while True:
        sys.stdout.write(repr(mote.read(1)))
        sys.stdout.flush()

#---------------------------------------------------------------------------
#---------------------------------------------------------------------------

# From packet-zep.c in wireshark:
# ZEP v2 Header will have the following format (if type=1/Data):
# |Preamble|Version| Type |Channel ID|Device ID|CRC/LQI Mode|LQI Val|NTP Timestamp|Sequence#|Reserved|Length|
# |2 bytes |1 byte |1 byte|  1 byte  | 2 bytes |   1 byte   |1 byte |   8 bytes   | 4 bytes |10 bytes|1 byte|
ZepHeader = ("EX"                   # Preamble
             + struct.pack("B", 2)) # Version


# ZEP v2
zpacket = ("EX" + chr(2) + chr(1) + chr(26) + (chr(0xaa)+chr(0xbb))
          + chr(0) + chr(0xff) 
          + chr(1)*8 + 2*(chr(0x12) +chr(0x34))
          + 10*chr(0)
          + chr(0x10) + 0x10 * chr(0x11))


#0-

ZepPort = 17754 # 
OperaSnifferPort = 5000

FreqHigh = (244*32768) # TimerB frequency (almost 8 MHz)
FreqLow = (32768) # TimerB frequency (32 KiHz)

class MoteSniffer:
    def __init__(self, mote, option):
        self.mote = mote
        self.sd = None
        self.channel = 26
        self.lastSfdUp = None
        self.option = option

    #--------------------------------------------------

    def runAsPacketSniffer(self, outputFormat = "opera", packetObserver = None):
        if self.option.logFileName != None:
            self.openLog()
        else: self.log = None

        self.outputFormat = outputFormat
        self.packetObserver = packetObserver
        self.prepareOutputFormat()
        #mote.write(makeCmd("C"+chr(self.channel)))
        mote.write(makeCmd("P"))
        self.loopPacketSniffer()

    def openLog(self):
        self.log = open(self.option.logFileName, "wb")
        self.log.write("version %s.%s\n" % (LogVersion[0], LogVersion[1]))
        self.log.write("channel %s\n" % self.channel)
        self.log.write("tty %s\n" % self.mote.ttyName)
        self.log.write("start-time %lf\n" % time.time())
        self.log.write("end-header\n")
        self.log.flush()

    def prepareOutputFormat(self):
        if self.outputFormat == "opera" or self.outputFormat == "wireshark":
            if self.sd != None:
                self.sd.close()
                del self.sd
            self.sd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            if self.outputFormat == "wireshark":
                self.sd.bind(("", ZepPort))

    def loopPacketSniffer(self):
        self.baseTime = None
        while True:
            data = moteGetCmdAnswer(mote)
            if data == 'P': 
                print "* sniffer mote acknowledged capture mode"
                continue
            elif data == 'C':
                print "* sniffer mote switched to canal", repr(data)
                continue
            elif data[0] == 'p':
                if self.outputFormat != "observer":
                    sys.stdout.write("+") ; sys.stdout.flush()
                if self.outputFormat == "wireshark": self.sendAsZep(data)
                elif self.outputFormat == "text": self.dumpAsText(data)
                elif self.outputFormat == "record": self.recordPacket(data)
                elif self.outputFormat == "observer": 
                    if self.packetObserver != None: self.notifyObserver(data)
                else: self.sendAsOpera(data)
            elif data[0] == 's':
                self.processSfd(data)
            else:
                sys.stdout.write("?")
                sys.stdout.flush()

    def processSfd(self, data):
        assert data[0] == 's'
        #print "SFD", repr(data), len(data)
        if len(data[0]) == 1: 
            #sys.stdout.write("/") 
            #sys.stdout.flush()
            return
        isUp, padding, clock, serialClock = struct.unpack("<BBII", data[1:])
        #print "(",isUp, clock,")"
        info = ("sfd", isUp, clock)
        #if self.log != None:
        #    self.log.write(repr(info)+"\n")
        if self.log != None:
            self.log.write("sfd %s %s\n" % (isUp, clock))
            self.log.flush()
        #assert serialClock > clock
        if isUp:
            if self.lastSfdUp == None:
                self.lastSfdUp = clock
            else:
                sys.stdout.write("?")
                sys.stdout.flush()
        else:
            if self.lastSfdUp != None:
                #print clock - self.lastSfdUp 
                self.lastSfdUp = None
            else:
                sys.stdout.write("?")
                sys.stdout.flush()

    def sendAsOpera(self, data):
        Coef = 32000000 #/ (8000000/32768)
        lost, rssi, linkQal, counter, t = struct.unpack("<BBBII", data[1:12])
        #timestamp = long((time.time() - startSnifferTime) * Coef)
        #timestamp = t*4 
        timestamp = t * (Coef / 32768)
        packet = data[12:]
        msg = struct.pack("<BIQB", 0, counter, timestamp, len(packet)) +packet
        self.sd.sendto(msg, ("", OperaSnifferPort))

    def sendAsZep(self, data):
        lost, rssi, linkQal, counter, t = struct.unpack("<BBBII", data[1:12])
        #timestamp = long((time.time()))*long(long(1)<<long(32))
        timestamp = t * 537 # 7998076 Hz instead of 7995392 Hz (244*32768)
        
        packet = data[12:]
        channel = self.channel
        packetType = 1 # data
        msg = (ZepHeader 
               + struct.pack("!BBHBBQI", packetType, channel, moteId,
                             0, # LQI Mode
                             linkQal, # LQI
                             timestamp, counter))
        msg += 10*chr(0) # reserved
        msg += struct.pack("<B", len(packet)+2) + packet + chr(0xff)*2
        self.sd.sendto(msg, ("", ZepPort))


    def dumpAsText(self, data):
        lost, rssi, linkQual, counter, t = struct.unpack("<BBBII", data[1:12])
        if lost & 1 == 0: freq = FreqLow
        else: freq = FreqHigh
        timestamp = (t / float(freq), t)
        packet = data[12:]
        print ("timestamp=%s pkt#%d rssi=%d linkQual=%d len=%d" % (
                timestamp, counter, rssi, linkQual, len(packet)))
        if not self.option.shortInfo:
            print (" " + " ".join(["%02x"%ord(x) for x in packet]))
        #if self.log != None:
        #    packetHash = hashlib.md5(packet).hexdigest()
        #    info = ("packet", packetHash, t, rssi, linkQual, counter)
        #    self.log.write(repr(info)+"\n")

    def recordPacket(self, data):
        lost, rssi, linkQual, counter, t = struct.unpack("<BBBII", data[1:12])
        if lost & 1 == 0: freq = FreqLow
        else: freq = FreqHigh
        #timestamp = (t / float(freq), t)
        packet = data[12:]
        #print ("timestamp=%s pkt#%d rssi=%d linkQual=%d len=%d" % (
        #        timestamp, counter, rssi, linkQual, len(packet)))
        packetRepr = "".join(["%02x"%ord(x) for x in packet])
        packetHash = hashlib.md5(packet).hexdigest()
        self.log.write("packet " + packetHash 
                       + " %lf" % time.time()
                       + " %lf" % (t/float(freq))
                       + " %s %s %s" % (rssi, linkQual, counter)
                       + " " + packetRepr
                       + "\n")
        self.log.flush()


    def notifyObserver(self, data):
        assert self.packetObserver != None
        info = {"machineClock": time.time()}
        (info["lost"], info["rssi"], info["linkQual"], info["counter"], 
         t) = struct.unpack("<BbBII", data[1:12])
        info["rssi"] += -45 # cf. RSSI_OFFSET - cc2420 doc
        if info["lost"] & 1 == 0: freq = FreqLow
        else: freq = FreqHigh
        info["clock"] = (t/float(freq))
        info["packet"] = data[12:]
        self.packetObserver(info)

    #--------------------------------------------------

    def runAsRssiSniffer(self):
        mote.write(makeCmd("R"))
        while True:
            data = moteGetCmdAnswer(mote)
            if data == 'R': 
                print "* sniffer mote acknowledged rssi mode"
                continue
            elif data == 'C':
                print "* sniffer mote switched to canal", repr(data)
                continue
            elif data[0] == 'r':
                self.dumpRssiData(data)
            else:
                sys.stdout.write("?")
                sys.stdout.flush()
        
    def dumpRssiData(self, data):
        cmdLen = struct.calcsize("<BIB")
        code, timestamp, rssi = struct.unpack("<BIB", data[0:cmdLen])
        print timestamp, rssi
        #print code

    #--------------------------------------------------

    def runAsRssiToDac(self):
        mote.write(makeCmd("D"))
        while True:
            data = moteGetCmdAnswer(mote)
            if data == 'D':
                print "* sniffer mote acknowledged rssi-to-dac mode"
                continue
            elif data == 'C':
                print "* sniffer mote switched to canal", repr(data)
                continue
            else:
                sys.stdout.write("?")
                sys.stdout.flush()

    #--------------------------------------------------

    def setChannel(self, channel):
        cmd = "C" + struct.pack("B", channel)
        self.mote.port.write(makeCmd(cmd))
        answer = moteGetCmdAnswer(mote)
        if answer != cmd: 
            raise ValueError("FATAL, unexpected command answer':", 
                             (cmd, answer))
        self.channel = channel

#---------------------------------------------------------------------------

class RecordedPort:
    def __init__(self, recordedMote):
        self.recordedMote = recordedMote
    def inWaiting(self): 
        return self.recordedMote.mote.port.inWaiting()
    def read(self,*args): 
        data = self.recordedMote.mote.port.read(*args)
        info = (time.time(), "recv", data)
        self.recordedMote.f.write(repr(info)+"\n")
        self.recordedMote.f.flush()
        return data
    def write(self, data): 
        info = (time.time(), "send", data)
        self.recordedMote.f.write(repr(info)+"\n")
        self.recordedMote.f.flush()
        return self.recordedMote.mote.port.write(data)

class RecordedMote:
    def __init__(self, mote, fileName):
        s = datetime.datetime.fromtimestamp(time.time()).isoformat(sep="-")
        s = s.split(".")[0].replace(":","_")
        fileName = fileName.replace("DATE", s)
        self.fileName = fileName
        self.mote = mote
        self.f = open(fileName, "w")
        print "* recording in file '%s'" % fileName
        self.port = RecordedPort(self)

    def reset(self): return self.mote.reset()

    def openPort(self, speed=115200): return self.mote.openPort()

    def flush(self): return self.mote.flush()

    def write(self,*args): return self.mote.write(*args)

    def reOpenPort(self,*args): return self.mote.reOpenPort(*args)


#---------------------------------------------------------------------------

random.seed(time.time())

DefaultMoteId = 10

parser = optparse.OptionParser()
parser.add_option("--no-reset", dest="shouldNotReset", 
                  action="store_true",default=False)
parser.add_option("--mote-id", dest="moteId", type="int",
                  default=DefaultMoteId)
parser.add_option("--mote-type", dest="moteType", type="string",
                  default=None)
parser.add_option("--tty", dest="ttyName", action="store", default=None)
#parser.add_option("--bps", dest="speed", type="int",
#                  default=115200)
parser.add_option("--high-speed", dest="withHighSpeed", action="store_true",
                  default=False)
parser.add_option("--channel", dest="channel", action="store", type="int",
                  default=None)
parser.add_option("--log", dest="logFileName", action="store", type="string",
                  default=None)
parser.add_option("--exp", dest="expName", action="store", type="string",
                  default=None)
parser.add_option("--short", dest="shortInfo", action="store_true", 
                  default=False)
parser.add_option("--record", dest="recordFileName", action="store", 
                  type="string", default=None)

option,argList = parser.parse_args()

moteId = option.moteId
manager = MoteManager.MoteManager(contikiDir = "../..")

if option.ttyName != None:
    mote = manager.getMoteByTty(option.ttyName, option.moteType)
elif sys.platform == "darwin":
    ttyName = "/dev/tty.SLAB_USBtoUART"
    print "(MacOS detected, uing default tty '%s')" % ttyName
    mote = manager.getMoteByTty(ttyName, option.moteType)
else:
    mote = manager.getMoteById(moteId)

if option.recordFileName != None:
    mote = RecordedMote(mote, option.recordFileName)

#--------------------------------------------------

HighSpeed = 2000000 # 2 Mbps

if not option.shouldNotReset:
    mote.reset()
    time.sleep(0.1)

mote.openPort(speed=115200)
mote.flush()
isSync = attemptSyncMote(mote, 0.2, 15)
if not isSync:
    raise RuntimeError("Cannot sync with sniffer mote")
print "* connected to sniffer mote"

if option.withHighSpeed:
    print "* switching UART to high speed.",
    mote.port.write(makeCmd("H"))
    time.sleep(0.1)
    mote.reOpenPort(HighSpeed)
    isSync = attemptSyncMote(mote, 0.2, 15)
    if not isSync:
        raise RuntimeError("Cannot sync with sniffer mote")
    print "done"

#--------------------------------------------------

NbPacketStat = 20

MovingAvgCoef = 0.9
MovingAvgCoefList = [MovingAvgCoef ** i for i in range(NbPacketStat)]

print MovingAvgCoefList

class NodeInfo:
    
    def __init__(self, nodeId):
        self.nodeId = nodeId
        self.lastClock = None
        self.lastPacketId = None
        self.status = {}

    def addPacket(self, clock, machineClock, packetId, delay, rssi):
        #print clock, machineClock, packetId, delay
        self.updateClock(machineClock, packetId)
        self.status[packetId] = rssi
        self.lastPacketId = packetId
        self.lastClock = machineClock
        self.lastDelay = delay
        #print self.status

    def updateClock(self, machineClock, packetId = None):
        if self.lastClock == None:
            return
        if packetId != None and self.lastPacketId+1 == packetId:
            return # no loss
        # removed expired entries
        if packetId == None:
            #print self.lastClock,machineClock
            pass

        for key in self.status.keys():
            if (key > self.lastPacketId 
                or key < self.lastPacketId - NbPacketStat + 1):
                del self.status[key]

        while self.lastClock + self.lastDelay * 0.02 < machineClock:
            self.status[self.lastPacketId] = None
            self.lastPacketId += 1
            self.lastClock += self.lastDelay * 0.01



    def getStat(self):
        if len(self.status) == 0: return None
        packetIdList = sorted(self.status.keys())
        maxPacketId = max(packetIdList)
        total = 0
        base = 0
        strRecv = ""
        count = 0
        recv = 0
        for packetId in packetIdList:
            if packetId >= maxPacketId - NbPacketStat + 1:
                count += 1
                rssi = self.status[packetId]
                if rssi != None:
                    coef = MovingAvgCoefList[maxPacketId - packetId]
                    total += coef * rssi
                    base += coef
                    strRecv += "*"
                    recv += 1
                else: 
                    strRecv += "."
        if recv == 0: 
            avgRssi = -99
        else: avgRssi = total / float(base)
        #print 
        return (strRecv, recv, avgRssi, maxPacketId, self.status[maxPacketId])

#WithCurses = False
WithCurses = True

class SnifferBar:
    def __init__(self):
        self.nodeTable = {}
        self.ignoredPacket = 0
        self.incorrectPacket = 0
        self.lock = threading.Lock()

    def handlePacket(self, info):
        self.lock.acquire()
        self._handlePacket(info)
        self.lock.release()

    def _handlePacket(self, info):
        packet = info["packet"]
        #print (info, " " + " ".join(["%02x"%ord(x) for x in info["packet"]]))
        if len(packet) != 18: 
            # ignore packet
            self.ignoredPacket += 1
            return
        (panId, broadcastAddress, zero, senderId, rimeChannel 
         ) = struct.unpack("HHBBB", packet[3:3+7])
        #print  ("%x"%panId, broadcastAddress, senderId, rimeChannel)
        packetId, nodeId, delay = (struct.unpack("IHB", packet[11:11+7]))
        if (panId != 0xabcd or broadcastAddress != 0xffff or senderId != nodeId
            or rimeChannel != 0xee):
            self.incorrectPacket += 1  

        if nodeId not in self.nodeTable:
            self.nodeTable[nodeId] = NodeInfo(nodeId)
        self.nodeTable[nodeId].addPacket(
            info["clock"], info["machineClock"], packetId, delay, info["rssi"])
        #print packetId, nodeId, info["clock"], delay

    def startDisplayThread(self):
        self.displayThread = threading.Thread(target=self.runDisplayThread)
        self.displayThread.daemon = True
        self.displayThread.start()

    def runDisplayThread(self):
        if WithCurses:
            self.screen = curses.initscr()
            self.screen.border()
            curses.curs_set(0)
            #curses.noecho()
            #curses.cbreak()
            #self.screen.keypad(1)
            atexit.register(self.endCurses)
        while True:
            #print "(update)"
            time.sleep(0.6)
            self.lock.acquire()
            self.display()
            self.lock.release()

    def display(self):
        for i,nodeId in enumerate(sorted(self.nodeTable.keys())):
            node = self.nodeTable[nodeId]
            node.updateClock(time.time())
            #print node.status
            #self.screen.move(1,1+i)
            bar,nbRecv, avgRssi, lastPacketId, lastRssi = node.getStat()
            if lastRssi == None: lastRssi = -99
            info = bar.rjust(NbPacketStat) + " % 2d " % node.nodeId
            info += " % 3.2f [% 3d] % 5d" % (avgRssi, lastRssi, lastPacketId)
            info += " " + ("#" * int((lastRssi+99)/4)).ljust(99/4)
            info += "  "
            if WithCurses:
                self.screen.addstr(1+i,2, info)
                self.screen.refresh()
            else: print(info)

    def endCurses(self):
        curses.nocbreak()
        self.screen.keypad(0)
        curses.echo()
        curses.endwin()

#--------------------------------------------------

if len(argList) == 0:
    print "# no command, exiting"
    sys.exit(0)

sniffer = MoteSniffer(mote, option)
command = argList[0]

if option.channel != None:
    print "* switching to channel %s... " % option.channel,
    channel = option.channel
    if channel < 0: channel = 0
    if channel > 26: channel = 26
    sniffer.setChannel(channel)
    print "done"

if command == "sniffer-wireshark":
    sniffer.runAsPacketSniffer("wireshark")
elif command == "sniffer-opera":
    sniffer.runAsPacketSniffer("opera")
elif command == "sniffer-text":
    sniffer.runAsPacketSniffer("text")
elif command == "sniffer-bar":
    snifferBar = SnifferBar()
    snifferBar.startDisplayThread()
    sniffer.runAsPacketSniffer("observer", snifferBar.handlePacket)
elif command == "record":
    if option.logFileName == None:
        if option.expName == None:
            raise RunTimeError("Specify either --log or --exp")
        ttyName = os.path.basename(mote.ttyName)
        channel = sniffer.channel
        logFileName = "exp-%s-%s-ch%d.log" % (option.expName, ttyName, channel)
        option.logFileName = logFileName
    print("* using log file name '%s'" % option.logFileName)
    sniffer.runAsPacketSniffer("record")
elif command == "rssi":
    sniffer.runAsRssiSniffer()
elif command == "rssi-dac":
    sniffer.runAsRssiToDac()
elif command == "version":
    pass
else:
    print ("FATAL, unknown command: %s" % command)

#---------------------------------------------------------------------------
