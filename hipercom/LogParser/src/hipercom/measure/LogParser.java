//---------------------------------------------------------------------------
// Packet Sniffer
// Author: Cedric Adjih (Inria)
//---------------------------------------------------------------------------
// Copyright (c) 2012-2014, Inria
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//---------------------------------------------------------------------------

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
			packet.machineTime -= parser.startTime; // adjust time
			System.out.println(packet);
		}
	}
}
