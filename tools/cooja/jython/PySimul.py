#---------------------------------------------------------------------------
#                      Base Module for Jython script
#---------------------------------------------------------------------------
# Author: Cedric Adjih <Cedric.Adjih@inria.fr>
# Copyright 2012-2013 Inria
# All rights reserved
#...........................................................................
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the Institute nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#---------------------------------------------------------------------------

"""
Note: when the script gets called the following is available

Available Java variables:
  log    (scriptLog)
  global (hash)
  sim    (simulation)
  gui    (simulation.getGUI())
  node    (scriptMote)

Updated on each message:
  mote, id, time, msg
  coojaEventType, coojaEventData
"""

#---------------------------------------------------------------------------

import java.lang.String, java.lang.Class, java.io.File
import array.array, java.util.ArrayList, java.lang.System
import java.lang.RuntimeException as RuntimeException
import traceback
import os.path, sys, socket, struct, subprocess
import time as _time

import se.sics.cooja.radiomediums.UDGMConstantLoss as UDGMConstantLoss
import java.util.Observable, java.util.Observer

#---------------------------------------------------------------------------
# Extra toys
#---------------------------------------------------------------------------

import types

def recursiveDump(data, prefix, visited = set(), ignoredSet = set(),
                  printFunc = java.lang.System.out.println):
    methodList = set([])
    for name in dir(data):
        if name in ignoredSet:
            continue

        fullName = prefix+"."+name
        try: 
            value = getattr(data,name)
        except: 
            printFunc("%s: <exception while getting value>" % fullName)
            continue
        valueType = type(value)

        if valueType == None:
            printFunc("%s = %s" % (fullName, repr(value)))
        elif name == "__class__" or name == "class" or name == "__doc__":
            continue
        elif valueType is types.BuiltinFunctionType:
            continue
        elif valueType is types.MethodType:
            methodList.add(fullName)
        elif valueType is array.array:
            printFunc("%s[%s]:" % (fullName, len(value)))
            for i in range(len(value)):
                recursiveDump(value[i], fullName+"[%s]"%i, visited, ignoredSet,
                              printFunc)
                if len(value)>2:
                    break # only display the first one(s)
        elif valueType is java.util.ArrayList:
            printFunc("%s[%s]:" % (fullName, value.size()))
            for i in range(value.size()):
                recursiveDump(value.get(i), fullName+"[%s]"%i, 
                              visited, ignoredSet, printFunc)
                if value.size()>2:
                    break # only display the first one(s)
        elif valueType in [types.BooleanType, types.UnicodeType, 
                           types.StringType, types.FloatType,
                           types.NoneType, types.LongType, types.IntType]:
            printFunc("%s = %s" % (fullName, repr(value)))
        elif valueType.__class__ == java.lang.Class:
            
            if value in visited: 
                return
            visited.add(value)

            className = valueType.__class__.getName(valueType)
            if className.startswith("java."):
                printFunc("%s: <%s>" % (fullName, className))
            else:
                printFunc("%s <%s>:" % (fullName, className))
                recursiveDump(value, fullName, visited, ignoredSet, printFunc)
        else:
            printFunc("%s: UNKNOWN %s %s" % (
                fullName, valueType, valueType.__class__))

    for fullName in methodList:
        printFunc("%s(...)" % fullName)

#---------------------------------------------------------------------------
# Wireshark/ZEP sending: copied from contiki/hipercom/analysis/LogFormat.py
#---------------------------------------------------------------------------

# From packet-zep.c in wireshark:
# ZEP v2 Header will have the following format (if type=1/Data):
# |Preamble|Version| Type |Channel ID|Device ID|CRC/LQI Mode|LQI Val|NTP Timestamp|Sequence#|Reserved|Length|
# |2 bytes |1 byte |1 byte|  1 byte  | 2 bytes |   1 byte   |1 byte |   8 bytes   | 4 bytes |10 bytes|1 byte|
#ZepHeader = struct.pack("BBB", ord('E'), ord('X'), 2) # Preamble + Version
#^^^^ is an unicode string in Jython
ZepPort = 17754 

#--------------------------------------------------

# http://stackoverflow.com/questions/8244204/ntp-timestamps-in-python
#import datetime
#SYSTEM_EPOCH = datetime.date(*_time.gmtime(0)[0:3])
#NTP_EPOCH = datetime.date(1900, 1, 1)
#NTP_DELTA = (SYSTEM_EPOCH - NTP_EPOCH).days * 24 * 3600

NTP_DELTA = float(2208988800L) # XXX: fix NTP timestamp handling

#--------------------------------------------------

class WiresharkSender:
    def __init__(self, refTime = 0):
        self.sd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sd.bind(("", ZepPort))
        self.counter = 0
        self.notified = False
        self.refClock = (int(refTime)//(60*60*24))*(60*60*24)-60*60

    def sendAsZep(self, packetInfo):
        clock, moteId, packet, channel, power = packetInfo
        linkQual = power
        self.counter += 1
        timestamp = long((clock + self.refClock + NTP_DELTA) * (2 ** 32))
    
        packetType = 1 # data

        Zep1 = ord('E')
        Zep2 = ord('X')
        ZepVer = 2
        msg = struct.pack("!BBBBBHBBQI", 
                          Zep1, Zep2, ZepVer,
                          packetType, channel, moteId,
                          0, # LQI Mode
                          linkQual, # LQI
                          timestamp, self.counter)
        msg += 10*chr(0) # reserved
        msg += struct.pack("<B", len(packet)+2) + packet + chr(0xff)*2
        self.sd.sendto(msg, ("", ZepPort))

    def __call__(self, packetInfo): # used as a function
        if not self.notified: 
            print "PySimul: sending packets as ZEP on loopback (use: wireshark -i lo -k)"
            self.notified = True
        self.sendAsZep(packetInfo)

#---------------------------------------------------------------------------

class PyRadioMediumObserver(java.util.Observer):
    def __init__(self, simul, radioMedium, callbackFunction):
        self.simul = simul
        self.radioMedium = radioMedium
        self.callbackFunction = callbackFunction

    def update(self, obs, obj):
        connection = self.radioMedium.getLastConnection()
        if connection == None:
            return

        source = connection.getSource()
        packet = source.getLastPacketTransmitted().getPacketData()
        clock = self.simul.getCooja().getSimulationTime() /float (SEC)

        # ./apps/mspsim/src/se/sics/cooja/mspmote/interfaces/Msp802154Radio.java
        power = int(255 * source.getCurrentOutputPowerIndicator()
                   /float(source.getOutputPowerIndicatorMax()))
        power = max(0, min(255, power))
        sourceId = source.getMote().getID()
        channel = source.getChannel()
        self.callbackFunction( (clock, sourceId, packet.tostring(), 
                                channel, power) )

#---------------------------------------------------------------------------

SEC=1000000
MILLISEC=1000

class DefaultSimulObserver:
    def __init__(self, pySimul, commandArgList, editorScriptInfo):
        self.pySimul = pySimul
        self.commandArgList = commandArgList
        self.editorScriptInfo = editorScriptInfo
        print "WARNING: script did not set the simulation observer (loadScript)"

    def onSimulStart(self, event):
        print "[DefaultSimulObserver] random seed is: %s" \
            % self.pySimul.cooja.getRandomSeed()

    def onSimulStop(self, event):
        print "[DefaultSimulObserver] simulation is stopped"

    def onSimulMoteMessage(self, event):
        msg = java.lang.String(event["msg"])
        event["node"].setMoteMsg(event["mote"], event["msg"]) 
        event["log"].log("|%s"% event["time"]+"|"+repr(event["id"]) 
                         +"|"+event["msg"]+"\n")

#--------------------------------------------------

GlobalList = [
    "mote", "id", "time", "msg",
    "coojaEventType", "coojaEventData",
    "log", "sim", "gui", "node"
]

#--------------------------------------------------

class PySimul:
    def __init__(self, observerFactory, observerFactoryData, globalNameSpace):
        if observerFactory == None:
            self.observerFactory = DefaultSimulObserver
        else: self.observerFactory = observerFactory
        self.observerFactoryData = observerFactoryData
        self.globalNameSpace = globalNameSpace
        self.observer = None

        self.processList = []

    #--------------------------------------------------
    # Main loop
    #--------------------------------------------------

    def _getGlobalInfo(self):
        result = {}
        for name in GlobalList:
            if name in self.globalNameSpace:
                result[name] = self.globalNameSpace[name]
        return result
        
    def runLoop(self):
        g = self.globalNameSpace

        g["SEMAPHORE_SIM"].acquire()
        g["SEMAPHORE_SCRIPT"].acquire()

        self.cooja = g["sim"]
        self.javaSim = self.cooja # alias
        self.sim = self.cooja     # alias
        self.log = g["log"]
        
        if g["SHUTDOWN"] or g["TIMEOUT"]:
            return self.handleEnd(g["SHUTDOWN"], g["TIMEOUT"])
        
        try:
            self.observer = self.observerFactory(
                self, self.cooja.getArgList(), self.observerFactoryData)
            self.observer.onSimulStart(self._getGlobalInfo())
        except:
            g["SEMAPHORE_SIM"].release(100)
            g["SEMAPHORE_SCRIPT"].release(100)
            raise
        
        while True:
            if g["SHUTDOWN"] or g["TIMEOUT"]:
                return self.handleEnd(g["SHUTDOWN"], g["TIMEOUT"])

            if g["coojaEventType"] == "<schedule>":
                g["coojaEventData"]()
            else: self.observer.onSimulMoteMessage(self._getGlobalInfo())

            g["SEMAPHORE_SIM"].release()
            g["SEMAPHORE_SCRIPT"].acquire()


    def handleEnd(self, shutdown, timeout):
        g = self.globalNameSpace
        
        if self.observer != None:
            try: 
                self.observer.onSimulStop(self._getGlobalInfo())
                self.killEachSubProcess()
            except: 
                g["SEMAPHORE_SIM"].release(100)
                raise
        if shutdown:
            self.eventShutdown()
            g["SEMAPHORE_SIM"].release(100)
            return
        if timeout:
            self.eventTimeOut()
            g["log"].testOK()
            g["SEMAPHORE_SIM"].release(100)
            return


    def eventShutdown(self):
        self.log.log('SHUTDOWN\n')

    def eventTimeOut(self):
        self.log.log('TIMEOUT\n')

    #--------------------------------------------------
    # Utility methods
    #--------------------------------------------------

    def scheduleEvent(self, delay, callback):
        self.log.generateMessageMicrosec(delay, "", "<schedule>", callback)

    def sendToMote(self, moteId, data):
        targetMote = self.getMote(moteId)
        targetMote.getInterfaces().getLog().writeString(data)

    def moveMote(self, moteId, x, y, z=0):
        targetMote = self.getMote(moteId)
        targetMote.getInterfaces().getPosition().setCoordinates(x,y,z)

    def getMote(self, moteId):
        if moteId == None or moteId == "<current>":
            return self.globalNameSpace["mote"]
        else: return self.sim.getMoteWithID(moteId)

    #--------------------------------------------------
    # Configuration / Path
    #--------------------------------------------------

    def getConfigDir(self):
        g = self.globalNameSpace
        return os.path.dirname(g["gui"].currentConfigFile.toString())

    def getContikiDir(self):
        g = self.globalNameSpace
        return g["contikiDir"]

    def execFileInConfigDir(self, fileName):
        print "PySimul: running script file '%s'" % fileName
        configDir = self.getConfigDir()
        if configDir not in sys.path:
            sys.path.append(configDir)
        fullFileName = os.path.join(self.getConfigDir(), fileName)
        execfile(fullFileName, globals(), globals())

    def execConfigPyScript(self):
        g = self.globalNameSpace
        configFileName = os.path.basename(g["gui"].currentConfigFile.toString())
        pyScriptFileName = configFileName.replace(".csc", ".py-script")
        self.execFileInConfigDir(pyScriptFileName)

    #--------------------------------------------------
    # This was a hack
    #--------------------------------------------------

    def setRadioLoss(self, coef2, coef1, coef0):
        # XXX: for an internal hack
        """let d = currentRange/maxRadioRange; then:
        receive_prob(d) = coef2 * d^2 + coef1 *d + coef0 (bounded in [0,1])"""
        radioMedium = sim.getRadioMedium()
        if isinstance(radioMedium, UDGMConstantLoss):
            radioMedium.coef2 = coef2
            radioMedium.coef1 = coef1
            radioMedium.coef0 = coef0
        else: raise RuntimeException("RadioMedium is not UDGMConstantLoss")

    #--------------------------------------------------
    # More utility functions
    #--------------------------------------------------

    def getCooja(self):
        return self.cooja

    def addRadioMediumObserver(self, callback):
        radioMedium = self.cooja.getRadioMedium()
        radioObserver = PyRadioMediumObserver(self, radioMedium, callback)
        radioMedium.addRadioMediumObserver(radioObserver)

    def setTimeOut(self, microsec):
        self.globalNameSpace["logScriptEngine"].setTimeOut(microsec)

    def startWiresharkZepSender(self):
        self.wiresharkSender = WiresharkSender(_time.time())
        self.addRadioMediumObserver(self.wiresharkSender)

    #--------------------------------------------------
    # Process Management
    #--------------------------------------------------

    def startSubProcessInTerm(self, title, command, bgColor="White"):
        ROXTerm = "/usr/bin/roxterm"
        command += " ; printf '<done>' ; sleep 10" # so that error can be seen
        if os.path.exists(ROXTerm):
            argList = [ROXTerm, "--tab", "-n", title,
                       "-e", "bash", "-c", command]
        else:
            argList = ["xterm", 
                       "-bg", bgColor, "-fg", "NavyBlue",
                       "-T", title,
                       "-e", "bash -c '%s'" % command]
        tunslipProcess = subprocess.Popen(args=argList, shell=False)
        self.processList.append(tunslipProcess)

    def addSubProcess(self, process):
        self.processList.append(process)

    def killEachSubProcess(self):
        for process in self.processList:
            # process.terminate()

            # from: http://python.6.n6.nabble.com/Re-Having-issues-using-subprocess-module-td4974706.html
            # process._process.destroy()

            # from further reading jython source code
            # jython-2.5.1/Lib/subprocess.py
            # (better would be to directly use java.lang.ProcessBuilder anyway)
            for ct in (process._stdin_thread, process._stdout_thread, 
                       process._stderr_thread):
                if ct != None:
                    ct.interrupt()
            process._process.destroy()

            del process

        self.processList = []

#---------------------------------------------------------------------------
