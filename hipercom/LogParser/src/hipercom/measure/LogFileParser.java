package hipercom.measure;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Iterator;

import javax.xml.bind.DatatypeConverter;


//The log format starts with the following lines:
//---------------
//version <x>.<y>
//channel <channel>
//tty <tty name>
//start-time <machine log start time>
//end-header
//---------------
//
//Then there are two kinds of entries "sfd" entries and "packet" entries.
//The packet entries are as follows:
//---------------
//packet <hash> <machine time> <mote time> <rssi> <linkQual> <counter> <packet>
//---------------
//with: 
//<counter> is a counter incremented for each packet received by the mote
//<hash> is a md5sum of the <packet>
//<rssi> is the rssi given by the CC2420
//<linkQual> is the link quality given by the CC2420
//<packet> is the packet dumped in hexadecimal
//
//
//The "sfd" entries are:
//---------------
//sfd <is up> <clock>
//---------------



public class LogFileParser implements Iterator<LoggedPacket> {

	//----------
	
	protected BufferedReader reader = null;
	protected String fileName = null;
	protected LoggedPacket currentPacket = null;
	
	String ttyName = null;
	int channel = -1;
	double startTime = -1;
	
	//----------
	
	LogFileParser(String fileName) {
		this.fileName = fileName;
	}
	
	Iterator<LoggedPacket> openFile() {
		// [java 1.7]: Path path = FileSystems.getDefault().getPath(fileName);
		assert( reader == null );
 
		try {
 			reader = new BufferedReader(new FileReader(fileName));
		} catch (IOException e) {
			e.printStackTrace();
			return null;
		}
				
		boolean isOk = readHeader();
		if (isOk)
			return this;
		else return null;
	}

	protected String readStrippedLine() {
		String line = null;
		try {
			line = reader.readLine();
		} catch (IOException e) {
			return null;
		}
		if (line == null)
			return null;
		line = line.replaceAll("^\\s", "").replaceAll("\\s$", "");
		if (line.startsWith("#") || line.length()==0)
			return readStrippedLine(); // ignore it and get another line [tail-recursion]
		return line;
	}
	
	protected boolean readHeader() {
		String line = null;
		for(;;) {
			line = readStrippedLine();
			if (line == null) {
				currentPacket = null; /* no data */
				return false;
			}
			assert( line.length() >= 1 );
			String[] tokenList = line.split("\\s");
			if (tokenList[0].equals("end-header"))
				break;
			else if (tokenList[0].equals("version")) {
				if (tokenList.length <= 1 || !tokenList[1].equals(LogParser.LogFormatVersion)) {
					System.err.println("Unsupported log file version: "+line);
					System.exit(1);
				}
			} else if (tokenList[0].equals("tty") && tokenList.length == 2) {
				ttyName = tokenList[1];
			} else if (tokenList[0].equals("channel") && tokenList.length == 2) {
				channel = Integer.parseInt(tokenList[1]);
			} else if (tokenList[0].equals("start-time") && tokenList.length == 2) {
				startTime = Double.parseDouble(tokenList[1]);
			} else {
				System.out.println("header> "+line);
			}
		} 
		prepareCurrentPacket();
		return true;
	}
	
	protected void prepareCurrentPacket() {
		String line = null;
		for (;;) {
			line = readStrippedLine();
			if (line == null) {	
				currentPacket = null;
				return;
			}
			if (line.startsWith("sfd"))
				continue;
			if (line.startsWith("packet"))
				break;
			System.out.println("ignored> "+line);
		}
		assert( line != null && line.startsWith("packet"));
		String tokenList[] = line.split("\\s"); 
		assert( tokenList[0].equals("packet") );
		if (tokenList.length != 8) {
			System.err.println("bad format> "+ line);
			currentPacket = null;
			return;
		}
		currentPacket = new LoggedPacket();
		// tokenList[1] == <hash>   -- is ignored
		currentPacket.machineTime = Double.parseDouble(tokenList[2]);
		currentPacket.moteTime = Double.parseDouble(tokenList[3]);
		currentPacket.rssi = Integer.parseInt(tokenList[4]);
		currentPacket.linkQual = Integer.parseInt(tokenList[5]);
		currentPacket.snifferPacketCounter = Integer.parseInt(tokenList[6]);
		currentPacket.packet = hexToBytes(tokenList[7]);
	}
	
	private byte[] hexToBytes(String string) {
		// http://stackoverflow.com/questions/140131/convert-a-string-representation-of-a-hex-dump-to-a-byte-array-using-java
		return DatatypeConverter.parseHexBinary(string);
	}

	@Override
	public boolean hasNext() {
		return currentPacket != null;
	}

	@Override
	public LoggedPacket next() {
		LoggedPacket result = currentPacket;
		prepareCurrentPacket();
		return result;
	}

	@Override
	public void remove() {
		/* not implemented */
	}
	
}
