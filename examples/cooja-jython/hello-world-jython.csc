<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project EXPORT="discard">[APPS_DIR]/mrm</project>
  <project EXPORT="discard">[APPS_DIR]/mspsim</project>
  <project EXPORT="discard">[APPS_DIR]/avrora</project>
  <project EXPORT="discard">[APPS_DIR]/serial_socket</project>
  <project EXPORT="discard">[APPS_DIR]/collect-view</project>
  <project EXPORT="discard">[APPS_DIR]/powertracker</project>
  <simulation>
    <title>SimpleHelloWorld</title>
    <speedlimit>10.0</speedlimit>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>se.sics.cooja.radiomediums.SilentRadioMedium</radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      se.sics.cooja.contikimote.ContikiMoteType
      <identifier>mtype44</identifier>
      <description>Cooja Mote Type #1</description>
      <source>[CONTIKI_DIR]/examples/cooja-jython/simple-hello.c</source>
      <commands>make simple-hello.cooja TARGET=cooja</commands>
      <moteinterface>se.sics.cooja.interfaces.Position</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.Battery</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiVib</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiBeeper</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiIPAddress</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiButton</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiPIR</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiClock</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiLED</moteinterface>
      <moteinterface>se.sics.cooja.contikimote.interfaces.ContikiCFS</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.MoteAttributes</moteinterface>
      <symbols>false</symbols>
    </motetype>
    <mote>
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>4.604604604604605</x>
        <y>0.0</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.contikimote.interfaces.ContikiMoteID
        <id>1</id>
      </interface_config>
      <motetype_identifier>mtype44</motetype_identifier>
    </mote>
    <mote>
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>95.3953953953954</x>
        <y>4.604604604604605</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.contikimote.interfaces.ContikiMoteID
        <id>2</id>
      </interface_config>
      <motetype_identifier>mtype44</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    se.sics.cooja.plugins.SimControl
    <width>280</width>
    <z>2</z>
    <height>160</height>
    <location_x>667</location_x>
    <location_y>240</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.Visualizer
    <plugin_config>
      <skin>se.sics.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>se.sics.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>se.sics.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <viewport>1.3090909090909089 0.0 0.0 1.3090909090909089 73.62545454545446 44.040000000000006</viewport>
    </plugin_config>
    <width>272</width>
    <z>3</z>
    <height>240</height>
    <location_x>667</location_x>
    <location_y>396</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <coloring />
    </plugin_config>
    <width>365</width>
    <z>0</z>
    <height>241</height>
    <location_x>592</location_x>
    <location_y>-4</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>#[python] # &lt;- Jython is used if script starts with "#[python" or "#[jython"&#xD;
&#xD;
# This is displayed in the terminal/console from where COOJA is started (e.g. "ant")&#xD;
print "** JythonSupportDir:", jythonSupportDir&#xD;
print "** logScriptEngine:", logScriptEngine&#xD;
print "** configPyScript:", configPyScript&#xD;
print "** configFileName:", configFileName&#xD;
print "** configDir:", configDir&#xD;
print "** contikiDir:", contikiDir&#xD;
&#xD;
class MySimulObserver:&#xD;
    def __init__(self, pySimul, commandLineArgs, observerData):&#xD;
        self.pySimul = pySimul&#xD;
        self.pySimul.setTimeOut(600*PySimul.SEC) # 600 seconds (in microseconds)&#xD;
        print "**** MySimulObserver created:"&#xD;
        print "****   pySimul:", pySimul&#xD;
        print "****   commandLineArgs:", commandLineArgs&#xD;
        print "****   observerData:", observerData&#xD;
    def onSimulStart(self, event):&#xD;
        print "**** Starting simulation"&#xD;
        self.updateMotePosition()&#xD;
    def onSimulStop(self, event):&#xD;
        print "**** Stopping simulation"&#xD;
    def onSimulMoteMessage(self, event):&#xD;
        print "**** Mote message event:", event	  &#xD;
    def updateMotePosition(self):&#xD;
        simTime = self.pySimul.getCooja().getSimulationTime()&#xD;
        t = (int(simTime / 10000.0) % 1000)/999.0&#xD;
        if t &gt; 0.5: t = 2*(1-t)&#xD;
        else: t = 2*t&#xD;
        self.pySimul.moveMote(1, 100*t, 0)&#xD;
        self.pySimul.moveMote(2, 100*(1-t), 100*t) &#xD;
        self.pySimul.scheduleEvent(PySimul.MILLISEC, self.updateMotePosition)&#xD;
        &#xD;
&#xD;
loader.setObserverFactory(MySimulObserver, ["some-data"])&#xD;
# Alternative: put everything here in a file instead and execute it&#xD;
#loader.loadScript(configPyScript, "SimulObserver", ["some-data"])</script>
      <active>true</active>
    </plugin_config>
    <width>657</width>
    <z>1</z>
    <height>671</height>
    <location_x>-1</location_x>
    <location_y>2</location_y>
  </plugin>
</simconf>

