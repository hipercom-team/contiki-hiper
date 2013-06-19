#---------------------------------------------------------------------------
# Test for JSR223
#---------------------------------------------------------------------------
# See also: http://www.jmdoudoux.fr/java/dej/chap-scripting.htm
# https://blogs.oracle.com/sundararajan/entry/java_integration_beanshell_and_jython

# OLD: from reading '/usr/bin/jython'
#export CLASSPATH=.:/usr/share/java/jython.jar:/usr/share/java/antlr3-runtime.jar:/usr/share/java/asm3.jar:/usr/share/java/asm3-util.jar:/usr/share/java/asm3-commons.jar:/usr/share/java/jruby.jar:/usr/share/java/jna.jar:/usr/share/java/jna-posix.jar:.:/usr/share/java/jython.jar

# standalone:
JYTHONJAR=../../dist/lib/jython.jar
export CLASSPATH=${JYTHONJAR}:.

all: run

TestJSR223.class: TestJSR223.java
	javac TestJSR223.java

${JYTHONJAR}: 
	cd ../.. && ant jython

run: TestJSR223.class ${JYTHONJAR}
	java TestJSR223

#---------------------------------------------------------------------------
