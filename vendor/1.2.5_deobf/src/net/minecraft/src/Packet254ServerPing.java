package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet254ServerPing extends Packet {
	public void readPacketData(DataInputStream var1) throws IOException {
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
	}

	public void processPacket(NetHandler var1) {
		var1.handleServerPing(this);
	}

	public int getPacketSize() {
		return 0;
	}
}
