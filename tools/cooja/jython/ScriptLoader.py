#---------------------------------------------------------------------------
#                      Loader for Jython-based scripts
#---------------------------------------------------------------------------
# Author: Cedric Adjih <Cedric.Adjih@inria.fr>
# Copyright 2013 Inria
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
The loader for Jython scripts

The functionning is as follows:
- 1) This Python file is executed at load by COOJA/Java if the
     simulation script starts with "#[python" or "#[jython"
- 2) This file has access to the following variables (set by Java side):
       - the editor script code written in the "Simulation script editor"
       - other paths
     but does nothing when it is executed the first time
- 2bis)
     Later a thread is started from the Cooja/Java side as a "Runnable",
     when the simulation is actually started ; this in turn results
     as a call the "run(...)" function from this module
- 3) the 'run(...)' fucntion executes the editor script code:
  - in the globals, the editor script code has acccess to same variables as
    in the step "2)":
    . the PySimul module (pre-imported)
    . loader: an instance of PySimulScriptLoader
    . configPyScript: name of simulation config file with suffix .csc
        replaced by suffix .py-script 
    . jythonSupportDir, configDir, contikiDir, configFileName
  - when it terminates, the editor script should have called one of:
    . loader.loadScript([<fileName> [, <factoryName> [, <factoryData>]]])
    . loader.setObserverFactory([<fileName>, [, factorName]])
- 4) PySimul.PySimul instance is created 
- 5) the PySimul instance follows the events of the simulation:
  . when simulation starts (just after it has started), it creates
    an instance of the 'simulation observer' by calling the factory 
    indicated at step 3) [if not set, a default one is used],
    with arguments: PySimul instance, the list of command line arguments,
                    and <factory data>
    and on this instance the methods are called:
  . <observer>.onSimulStart(event): at start
  . <observer>.onSimulStop(event): when the simulation is stopped
  . <observer>.onSimulMoteMessage(event): every time there is a mote message

The argument event is a dictionary with the following values:
   id (moteId), time, msg
and the extras (may be removed in the future):
   mote, log, sim, gui, node
   coojaEventType, coojaEventData

--------------------------------------------------
Internals of LogScriptRunner.java interfaces ("4)" below is also valid for
Javascript scripting)
Everything operates as follows:
1) This file is loaded by COOJA with an 'execfile'.
2) The 'scriptCode' of the simulation is then also executed by COOJA.
3) A thread is started which will call the run() function available in globals
   (hence the one defined in this file, unless overridden by the user)
4) The simulation will work by having the Java side and the Jython side
   alternatively acquiring and releasing some locks
   (this is the design choosen by COOJA for Javascript, nothing was changed).
   Information is passed to the script by setting global variables
     mote, id, time, msg (added: coojaEventType, coojaEventData)
   before releasing the lock. 
   Other variables are also available: log, global, sim, gui, node.
   (see PyCoojaTranslated.py for the approximate translation of the 
    sample Javascript script that justs logs everything - this was the starting
    point for this file).

Note: the thread-safeness has not been checked
"""

#---------------------------------------------------------------------------

# --- Notes on the use of PythonInterpreter:
# There are lot of problems trying to get every script code have a fresh
# Python without the former modules (loaded by previous instances). It's hard.
# This is necessary because the script writer might just modify a module
# that s/he imported, and expect it to be re-read when simulation is reloaded.
#
# Related references:
# [1] http://comments.gmane.org/gmane.comp.lang.jython.devel/4009
# [2] http://python.6.n6.nabble.com/Multiple-PythonInterpreters-reloading-modules-td1772538.html
# [3] http://underpop.free.fr/j/java/jython-for-java-programmers/_chapter%209.htm
#
# I don't know how to properly link Jython-standalone with COOJA, and then
# create a Jython PythonInterpreter from Java directly -- but as a awful+clever
# hack, one can be just created from Jython itself, started by ScriptEngine!
# (hope this will not create obscure problems)
#
# about [2] not sure it is still valid or if it is special when from Jython  -
# Py.getSystemState() seems to be the same before and after  'exec'

#---------------------------------------------------------------------------

try:
    # these are Java objects:
    from org.python.util import PythonInterpreter 
    from org.python.core import PySystemState, Py
    hasPythonInterpreter = True
except:
    print "Cannot import PythonInterpreter, PySystemState or Py"
    hasPythonInterpreter = False # XXX: not handled yet
    raise NotImplemented("running without org.python.util.PythonInterpreter")

#---------------------------------------------------------------------------

class PySimulScriptLoader:
    def __init__(self, context):
        self.context = context
        self.scriptCode = scriptCode
        self.pySystemState = PySystemState()
        self.interpreter = PythonInterpreter(context, self.pySystemState)
        self.observerFactory = None
        self.observerFactoryData = None
    
    def loadScript(self, fileName = None, factoryName = "SimulObserver",
                   factoryData = None):
        """This method is designed to be the main/only method called from
        the 'simulation script editor' in COOJA as:
          loader.loadScript(configPyScript [, "SimulObserver" [, {}]])

        . it loads (executes) Python code from the given file name ;
        . after what the Python simulation factory object with the
          name specified is looked up
        . then it is called as factory(<factoryData>)
        """
        # Note: we reuse the same PythonInterpreter
        oldState = Py.getSystemState() # because of [2], necessary?
        #self.interpreter.setLocals({})
        #self.pySystemState.initialize() # clean up
        self.interpreter.execfile(fileName)
        Py.setSystemState(oldState) # because of [2]
        observerFactory = self.interpreter.get(factoryName)
        self.setObserverFactory(observerFactory, factoryData)

    def setObserverFactory(self, observerFactory, factoryData):
        self.observerFactory = observerFactory
        self.observerFactoryData = factoryData

    def actuallyRunEditorScriptCode(self):
        self.actuallyRunEditorScriptCode = None # don't let the user err

        oldState = Py.getSystemState() # because of [2], necessary?
        self.pySystemState.path.append(configDir)
        self.pySystemState.path.append(jythonSupportDir)
        # because "exec" reserved word in python:
        interpreterExec = getattr(self.interpreter, "exec")
        interpreterExec("import PySimul") # don't want to import it in this py.
        interpreterExec(scriptCode)
        PySimul = self.interpreter.eval("PySimul")
        Py.setSystemState(oldState) # because of [2]

        pySimul = PySimul.PySimul(self.observerFactory, 
                                  self.observerFactoryData,
                                  globals())
        return pySimul

#---------------------------------------------------------------------------

def run(scriptCode = scriptCode):
    global jythonSupportDir, logScriptEngine, scriptEngine #available (not used)
    global currentConfigFile
    context = { 
        "jythonSupportDir": jythonSupportDir, 
        "logScriptEngine": logScriptEngine,
        "configPyScript": currentConfigFileName.replace(".csc", ".py-script"),
        "configFileName": currentConfigFileName,
        "configDir": configDir,
        "contikiDir": contikiDir
        }
    loader = PySimulScriptLoader(context)
    context["loader"] = loader
    pySimul = loader.actuallyRunEditorScriptCode()
    pySimul.runLoop()

#---------------------------------------------------------------------------


