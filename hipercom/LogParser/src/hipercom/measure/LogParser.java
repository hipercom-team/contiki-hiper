package hipercom.measure;

import java.util.Iterator;

public class LogParser {

	public static final Object LogFormatVersion = "1.0";

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		if (args.length < 1) {
			System.err.println("Syntax: <prog name> <log file name>");
			System.exit(1);
		}
		processLogFile(args[0]);
	}

	// Example of use of LogFileParser class
	public static void processLogFile(String fileName) {
		LogFileParser parser = new LogFileParser(fileName);
		Iterator<LoggedPacket> it = parser.openFile();
		
		if (it == null) {
			System.err.println("error opening file '"+fileName+"'");
			System.exit(1);
		}
		
		// write header
		System.out.println("--- Content of log file '"+fileName+"':");
		System.out.println("tty="+parser.ttyName);
		System.out.println("channel="+parser.channel);
		System.out.println("startTime="+parser.startTime);
		
		// write content
		while (it.hasNext()) {
			LoggedPacket packet = it.next();
			System.out.println(packet);
		}
	}
}
