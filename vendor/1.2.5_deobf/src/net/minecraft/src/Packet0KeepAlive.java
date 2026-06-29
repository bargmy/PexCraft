package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet0KeepAlive extends Packet {
	public int randomId;

	public Packet0KeepAlive() {
	}

	public Packet0KeepAlive(int var1) {
		this.randomId = var1;
	}

	public void processPacket(NetHandler var1) {
		var1.handleKeepAlive(this);
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.randomId = var1.readInt();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.randomId);
	}

	public int getPacketSize() {
		return 4;
	}
}
