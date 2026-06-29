package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet42RemoveEntityEffect extends Packet {
	public int entityId;
	public byte effectId;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.entityId = var1.readInt();
		this.effectId = var1.readByte();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.entityId);
		var1.writeByte(this.effectId);
	}

	public void processPacket(NetHandler var1) {
		var1.handleRemoveEntityEffect(this);
	}

	public int getPacketSize() {
		return 5;
	}
}
