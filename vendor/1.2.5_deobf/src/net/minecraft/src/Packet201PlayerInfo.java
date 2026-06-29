package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet201PlayerInfo extends Packet {
	public String playerName;
	public boolean isConnected;
	public int ping;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.playerName = readString(var1, 16);
		this.isConnected = var1.readByte() != 0;
		this.ping = var1.readShort();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		writeString(this.playerName, var1);
		var1.writeByte(this.isConnected ? 1 : 0);
		var1.writeShort(this.ping);
	}

	public void processPacket(NetHandler var1) {
		var1.handlePlayerInfo(this);
	}

	public int getPacketSize() {
		return this.playerName.length() + 2 + 1 + 2;
	}
}
