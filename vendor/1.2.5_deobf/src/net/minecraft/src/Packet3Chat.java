package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet3Chat extends Packet {
	public static int field_52010_b = 119;
	public String message;

	public Packet3Chat() {
	}

	public Packet3Chat(String var1) {
		if(var1.length() > field_52010_b) {
			var1 = var1.substring(0, field_52010_b);
		}

		this.message = var1;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.message = readString(var1, field_52010_b);
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		writeString(this.message, var1);
	}

	public void processPacket(NetHandler var1) {
		var1.handleChat(this);
	}

	public int getPacketSize() {
		return 2 + this.message.length() * 2;
	}
}
