package hipercom.measure;

import javax.xml.bind.DatatypeConverter;

public class LoggedPacket {
	double machineTime = -1;
	double moteTime = -1;
	int rssi = -1;
	int linkQual = -1;
	int snifferPacketCounter = -1;
	byte packet[] = null;
	
	public String toString() {
		return "packet"
				+"<t="+machineTime
				+",t="+moteTime
				+",rssi="+rssi
				+",qual="+linkQual
				+",id="+snifferPacketCounter
				+",data="+DatatypeConverter.printHexBinary(packet)
				+">";
	}
}
