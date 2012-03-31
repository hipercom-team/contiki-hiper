#---------------------------------------------------------------------------
#                           Packet Sniffer
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

import sys, time, select, optparse, random, struct, hashlib
import socket


sys.path.append("..")
import MoteManager

#---------------------------------------------------------------------------

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

Freq = (244*32768) # TimerB frequency

class MoteSniffer:
    def __init__(self, mote, option):
        self.mote = mote
        self.sd = None
        self.channel = 26
        self.lastSfdUp = None
        self.option = option
        if option.logFileName != None:
            self.log = open(option.logFileName, "wb")
        else: self.log = None

    #--------------------------------------------------

    def runAsPacketSniffer(self, outputFormat = "opera", channel=26):
        self.outputFormat = outputFormat
        self.prepareOutputFormat()
        #mote.write(makeCmd("C"+chr(channel)))
        mote.write(makeCmd("P"))
        self.loopPacketSniffer()

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
                sys.stdout.write("+") ; sys.stdout.flush()
                if self.outputFormat == "wireshark": self.sendAsZep(data)
                elif self.outputFormat == "text": self.dumpAsText(data)
                else: self.sendAsOpera(data)
            elif data[0] == 's':
                self.processSfd(data)
            else:
                sys.stdout.write("?")
                sys.stdout.flush()

    def processSfd(self, data):
        assert data[0] == 's'
        #print repr(data), len(data)
        isUp, padding, clock, serialClock = struct.unpack("<BBII", data[1:])
        #print "(",isUp, clock,")"
        info = ("sfd", isUp, clock)
        if self.log != None:
            self.log.write(repr(info)+"\n")
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
        Coef = 32000000
        lost, rssi, linkQal, counter, t = struct.unpack("<BBBII", data[1:12])
        #timestamp = long((time.time() - startSnifferTime) * Coef)
        timestamp = t*4 
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
        timestamp = (t / float(Freq), t)
        packet = data[12:]
        print ("timestamp=%s pkt#%d rssi=%d linkQual=%d len=%d" % (
                timestamp, counter, rssi, linkQual, len(packet)))
        #.ljust(10)
        if not self.option.shortInfo:
            print (" " + " ".join(["%02x"%ord(x) for x in packet]))
        if self.log != None:
            packetHash = hashlib.md5(packet).hexdigest()
            info = ("packet", packetHash, t, rssi, linkQual, counter)
            self.log.write(repr(info)+"\n")


    #--------------------------------------------------

    def runAsRssiSniffer(self, channel=26):
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
parser.add_option("--short", dest="shortInfo", action="store_true", 
                  default=False)

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

if option.channel != None:
    print "* switching to channel %s." % option.channel,
    channel = option.channel
    if channel < 0: channel = 0
    if channel > 26: channel = 26
    cmd = "C" + struct.pack("B", channel)
    mote.port.write(makeCmd(cmd))
    answer = moteGetCmdAnswer(mote)
    if answer != cmd: 
        print "FATAL, unexpected command answer':", repr(answer)
        sys.exit(1)        
    else: print "done"
    mote.channel = channel

#--------------------------------------------------

if len(argList) == 0:
    print "# no command, exiting"
    sys.exit(0)

sniffer = MoteSniffer(mote, option)
command = argList[0]

sniffer.lastTimeStamp = 0 #XXX

if command == "sniffer-wireshark":
    sniffer.runAsPacketSniffer("wireshark")
elif command == "sniffer-opera":
    sniffer.runAsPacketSniffer("opera")
elif command == "sniffer-text":
    sniffer.runAsPacketSniffer("text")
elif command == "rssi":
    sniffer.runAsRssiSniffer()
elif command == "rssi-dac":
    sniffer.runAsRssiToDac()
elif command == "version":
    pass
else:
    print ("FATAL, unknown command: %s" % command)

#---------------------------------------------------------------------------
