import javax.script.Invocable;
import javax.script.Compilable;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;

public class TestJSR223 {

    public static void main(String[] args) throws ScriptException {
        ScriptEngine engine = new ScriptEngineManager().getEngineByName("python");
        engine.eval("import sys");
	engine.eval("sys.path.append('/usr/share/jython/Lib')");
        engine.eval("import socket");
        engine.eval("import os");
        engine.eval("print sys");
        engine.eval("print os.listdir('.')");
        engine.put("a", 42);
        engine.eval("print a");
        engine.eval("x = 2 + 2");
        Object x = engine.get("x");
        System.out.println("x: " + x);

	System.out.println("(Invocable)engine: "+((Invocable)engine));
	System.out.println("invocable: "+ (engine instanceof Invocable));
	System.out.println("compilable: "+ (engine instanceof Compilable));
	engine.eval("from java.lang import Runnable");
	engine.eval(""
		    +"class MyThread(Runnable):\n"
		    +"  def run(*args):\n"
		    +"    print 'running'\n"
		    +"t = MyThread()\n"
		    +"def run(*args): print 'run module'\n"
		    );
	Runnable run = ((Invocable)engine).getInterface(Runnable.class);
	//System.out.println("Runnable: " + (run));
	run.run();
    }
}
