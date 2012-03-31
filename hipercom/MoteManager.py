#---------------------------------------------------------------------------
#                         Mote Management
#        Cedric Adjih, Hipercom Project-Team, Inria Paris-Rocquencourt
#                       Copyright 2012 Inria. 
#            All rights reserved. Distributed only with permission.
#---------------------------------------------------------------------------

import sys, optparse, subprocess, os, time, hashlib
import serial
from optparse import OptionParser

try:   
    from MoteDatabaseHipercom import MoteDatabase
except: MoteDatabase = {}

#---------------------------------------------------------------------------

DefaultContikiDir = ".."
DefaultMoteType = "z1"

#if not os.path.exists(ContikiDir):
#    ContikiDir = os.environ["HOME"]+"/Mote" + ContikiDir
#assert os.path.exists(ContikiDir) # update Contiki directory

#MoteBsl = ContikiDir+"/tools/z1/z1-bsl-nopic --z1"

#---------------------------------------------------------------------------

class MoteManager:
    def __init__(self, contikiDir = DefaultContikiDir, 
                 defaultMoteType = DefaultMoteType):
        self.contikiDir = contikiDir
        self.defaultMoteType = defaultMoteType

    def getMoteTableOfType(self, moteType):
        if moteType == None:
            moteType = self.defaultMoteType
        if moteType == "z1":
            command = [self.contikiDir+"/tools/z1/motelist-z1", "-c"]
        elif moteType == "sky":
            command = [self.contikiDir+"/tools/sky/motelist-linux", "-c"]
        else: raise ValueError("Unknown moteType", moteType)
        
        print "!", " ".join(command)
        moteStr = subprocess.Popen(command, 
                                   stdout=subprocess.PIPE).communicate()[0]
        result = []
        for line in moteStr.split("\n"):
            line = line.strip()
            if line == "": continue
            moteName,ttyName,model = line.split(",")
            result.append((moteName,(ttyName,moteType)))

        return dict(result)

    def getMoteTable(self):
        z1Table = self.getMoteTableOfType("z1")
        skyTable = self.getMoteTableOfType("sky")
        result = z1Table
        result.update(skyTable)
        return result

    def getMoteIdTable(self):
        return dict(( 
                (MoteDatabase[moteName], ttyAndType)
                for moteName,ttyAndType in self.getMoteTable().iteritems() ))

    def getMoteById(self, moteId):
        moteTable = self.getMoteIdTable()
        ttyName, moteType = moteTable[moteId] # EXCEPTION = NOT FOUND CONNECTED
        return self.getMoteByTty(ttyName, moteId, moteType)

    def getMoteByTty(self, ttyName, moteId = None, moteType = None):
        if moteType == None: 
            moteType = self.defaultMoteType
        return Mote(ttyName, moteId, self, moteType)

    def resetMoteByTty(self, ttyName, moteType=None):
        if moteType == None: 
            moteType = self.defaultMoteType
        print ("* Resetting mote " + ttyName)
        moteBsl = self.getBslCommand(moteType)
        command = moteBsl + " -c "+ttyName+" -r"
        print "! "+command
        subprocess.check_call(command.split(" "))

    def getBslCommand(self, moteType):
        if moteType == "z1":
            moteBsl = self.contikiDir+"/tools/z1/z1-bsl-nopic --z1"
        elif moteType == "sky":
            moteBsl = self.contikiDir+"/tools/sky/msp430-bsl-linux --telosb"
        else: raise ValueError("Unknown moteType", moteType)

        return moteBsl

#..................................................

class Mote:
    def __init__(self, ttyName, moteId = None, manager = None, moteType = None):
        self.ttyName = ttyName
        self.moteId = moteId
        self.manager = manager
        self.port = None

        #if moteType == None and manager != None:
        #    self.moteType = manager.defaultMoteType
        #else: self.moteType = moteType
        self.moteType = moteType

    def ensureNoPort(self):
        if self.port != None:
            self.port.close()
            del self.port
            self.port = None

    def reset(self):
        self.ensureNoPort()
        self.manager.resetMoteByTty(self.ttyName, self.moteType)

    def flashFirmware(self, z1Firmware):
        # See rules for "z1-u.%" "z1-upload" in
        #   'contiki/platform/z1/Makefile.common'
        self.ensureNoPort()

        if self.moteType == None:
            raise ValueError("No MoteType defined and not using default")
        moteBsl = self.manager.getBslCommand(self.moteType)

        firmware = "tmp-firmware.%s" % os.getpid()
        print ("**** Flashing mote " + self.ttyName + " with " + z1Firmware)

        print os.getcwd()
        command = "msp430-objcopy " + z1Firmware + " -O ihex " + firmware
        print "! "+command
        subprocess.call(command.split(" ")) 

        self.reset()

        command = moteBsl + " -c " + self.ttyName + " -e"
        print "! "+command
        subprocess.check_call(command.split(" "))
        time.sleep(2)
        
        command = (moteBsl 
                   + " -c " + self.ttyName 
                   + " -I -p " + firmware)
        print "! "+command
        subprocess.check_call(command.split(" "))
        time.sleep(2)

        self.reset()
        os.remove(firmware)

    def openPort(self, speed=115200):
        self.port = serial.Serial(port = self.ttyName, baudrate = speed)

    def reOpenPort(self, newSpeed=115200):
        if self.port != None: 
            self.port.close()
        self.openPort(newSpeed)

    def write(self, data):
        assert self.port != None
        self.port.write(data)

    def read(self, maxLength):
        return self.port.read(maxLength)

    def readWithTimeout(self, length, timeLimit = None):
        assert self.port != None
        result = ""
        startTime = time.time()
        while ((timeLimit == None or time.time()-startTime < timeLimit)
               and len(result) != length
               and self.port.inWaiting()):
            result +=  self.port.read(1)
        return result

    def flush(self, timeLimit=None):
        assert self.port != None
        result = ""
        startTime = time.time()
        while ((timeLimit == None or time.time()-startTime < timeLimit)
               and self.port.inWaiting()):
            result +=  self.port.read(1)
        return result

#---------------------------------------------------------------------------

def getMoteFromArg(argList, option):
    if option.moteId == None:
        raise RuntimeError("mote-id was not specified on cmd line")
    if option.moteId != None:
        moteId = int(option.moteId)
        manager = MoteManager(option.contikiDir)
        moteTable = manager.getMoteIdTable()
        if option.exitIfNoMote and (moteId not in moteTable):
            sys.exit(0)
        else: 
            ttyName, moteType = moteTable[moteId]
            mote = Mote(ttyName, moteId, manager, moteType)
    else: mote = Mote(option.ttyName, option.moteType)
    return mote

def doFlashCommand(argList, option):
    if option.moteId == None and option.moteType == None:
        raise RuntimeError("moteType was not specified on cmd line")
    if option.firmware == None:
        raise RuntimeError("firmware was not specified on cmd line")
    mote = getMoteFromArg(argList, option)

    if mote.moteId != None: hashFileName = ".last-flash-hash-%s" % mote.moteId
    else: hashFileName != None
    if mote.moteId != None and option.noUselessReflash and hashFileName != None:
        with open(option.firmware) as f:
            hexHash = hashlib.md5(f.read()).hexdigest()
            if os.path.exists(hashFileName):
                with open(hashFileName) as f:
                    if hexHash == f.read().strip():
                        print ( "-- mote %s "% mote.moteId
                                +"was already flashed with"
                                +" firmware '%s' (%s)"
                                %(option.firmware,hexHash[:8]))
                        return # already flashed with this firmware
    mote.flashFirmware(option.firmware)
    if hashFileName != None:
        with open(hashFileName, "w") as f:
            f.write(hexHash+"\n")
    _runAsCommand(argList, option)

def doDumpCommand(argList, option):
    mote = getMoteFromArg(argList, option)
    mote.openPort()
    while True:
        sys.stdout.write(mote.port.read(1))
        sys.stdout.flush()

def doResetCommand(argList, option):
    mote = getMoteFromArg(argList, option)
    mote.reset()
    _runAsCommand(argList, option) # continue

def _runAsCommand(argList, option):
    if len(argList) == 0:
        return
    name = argList[0]
    if name == "flash":
        doFlashCommand(argList[1:], option)
    elif name == "dump":
        doDumpCommand(argList[1:], option)
    elif name == "reset":
        doResetCommand(argList[1:], option)
    else: raise RuntimeError("bad command arg", name)

def runAsCommand(initialArgList):
    parser = OptionParser()
    parser.add_option("--mote-id", dest="moteId", default=None)
    parser.add_option("--tty", dest="ttyName", default=None)
    parser.add_option("--firmware", dest="firmware", default=None)
    parser.add_option("--ignore-no-mote", dest="exitIfNoMote", default=False,
                      action="store_true")
    parser.add_option("--avoid-useless-reflash", action = "store_true",
                      dest="noUselessReflash", default = False)
    parser.add_option("--mote-type", dest="moteType", default = None)
    parser.add_option("--contiki-dir", dest="contikiDir", 
                      default = DefaultContikiDir)
    option, argList = parser.parse_args(initialArgList)
    if len(argList) == 0:
        raise RuntimeError("no argument was provided on command line")
    _runAsCommand(argList, option)

#--------------------------------------------------

if __name__ == "__main__":
    runAsCommand(sys.argv[1:])

#---------------------------------------------------------------------------
