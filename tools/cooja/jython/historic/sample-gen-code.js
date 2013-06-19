/*---------------------------------------------------------------------------*/

 /*
 * Example Contiki test script (JavaScript).
 * A Contiki test script acts on mote output, such as via printf()'s.
 * The script may operate on the following variables:
 *  Mote mote, int id, String msg
 */

TIMEOUT(600000);

sim.getMoteWithID(2).getInterfaces().getPosition().setCoordinates(0.0,30.0,0.0);

GENERATE_MSG(10000, "wait"); /* wait for x seconds */
YIELD_THEN_WAIT_UNTIL(msg.equals("wait"));

log.log(time + ": starting logging ()\n");

write(sim.getMoteWithID(1),"\xc0!P\xAA\xAA\x00\x00\x00\x00\x00\x00\xc0")
GENERATE_MSG(30000, "wait"); /* wait for x seconds */
YIELD_THEN_WAIT_UNTIL(msg.equals("wait"));
/*write(sim.getMoteWithID(2),"\xc0!R\xc0");*/
sim.getMoteWithID(2).getInterfaces().getPosition().setCoordinates(60.0,30.0,0.0);

plugin = mote.getSimulation().getGUI().getStartedPlugin("se.sics.cooja.plugins.RadioLogger");
log.log("plugin: "+plugin);

while (true) {
  log.log(time + ":" + id + ":" + msg + "\n");
  YIELD();
}

/*---------------------------------------------------------------------------
 * Generated JavaScript Code
 *---------------------------------------------------------------------------*/

timeout_function = null; 
function run() { 
    SEMAPHORE_SIM.acquire(); 
    SEMAPHORE_SCRIPT.acquire(); 
    if (SHUTDOWN) { SCRIPT_KILL(); } 
    if (TIMEOUT) { SCRIPT_TIMEOUT(); } 
    msg = new java.lang.String(msg); 
    node.setMoteMsg(mote, msg); 
  
    sim.getMoteWithID(2).getInterfaces().getPosition().setCoordinates(0.0,30.0,0.0);
 
    GENERATE_MSG(10000, "wait"); 
 
    SCRIPT_SWITCH(); 
    while (!(msg.equals("wait"))) {  
	SCRIPT_SWITCH(); 
    };
 
    log.log(time + ": starting logging ()\n");
 
    write(sim.getMoteWithID(1),"\xc0!P\xAA\xAA\x00\x00\x00\x00\x00\x00\xc0")
    GENERATE_MSG(30000, "wait"); 
 
    SCRIPT_SWITCH(); while (!(msg.equals("wait"))) {
	SCRIPT_SWITCH(); 
    };
 
 
    sim.getMoteWithID(2).getInterfaces().getPosition().setCoordinates(60.0,30.0,0.0);
 
    plugin = mote.getSimulation().getGUI().getStartedPlugin("se.sics.cooja.plugins.RadioLogger");
    log.log("plugin: "+plugin);
 
    while (true) {
	log.log(time + ":" + id + ":" + msg + "\n");
	SCRIPT_SWITCH();
    }
 
 
    while (true) { 
	SCRIPT_SWITCH(); 
    } 
};

function GENERATE_MSG(time, msg) {  
    log.generateMessage(time, msg); 
};
 
function SCRIPT_KILL() {
    SEMAPHORE_SIM.release(100);  
    throw('test script killed'); 
};
 
function SCRIPT_TIMEOUT() { 
    if (timeout_function != null) 
    { timeout_function(); }  
    log.log('TEST TIMEOUT\n');  
    log.testFailed();  
    while (!SHUTDOWN) {   
	SEMAPHORE_SIM.release();   
	SEMAPHORE_SCRIPT.acquire();  
    }  
    SCRIPT_KILL(); 
};
 
function SCRIPT_SWITCH() {  
    SEMAPHORE_SIM.release();  
    SEMAPHORE_SCRIPT.acquire();  
    if (SHUTDOWN) { SCRIPT_KILL(); }  
    if (TIMEOUT) { SCRIPT_TIMEOUT(); }  
    msg = new java.lang.String(msg);  
    node.setMoteMsg(mote, msg); 
};
 
function write(mote,msg) {
    mote.getInterfaces().getLog().writeString(msg); 
};

/*---------------------------------------------------------------------------*/
