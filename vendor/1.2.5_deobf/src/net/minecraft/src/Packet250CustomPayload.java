package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet250CustomPayload extends Packet {
	public String channel;
	public int length;
	public byte[] data;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.channel = readString(var1, 16);
		this.length = var1.readShort();
		if(this.length > 0 && this.length < Short.MAX_VALUE) {
			this.data = new byte[this.length];
			var1.readFully(this.data);
		}

	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		writeString(this.channel, var1);
		var1.writeShort((short)this.length);
		if(this.data != null) {
			var1.write(this.data);
		}

	}

	public void processPacket(NetHandler var1) {
		var1.handleCustomPayload(this);
	}

	public int getPacketSize() {
		return 2 + this.channel.length() * 2 + 2 + this.length;
	}
}
