package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet41EntityEffect extends Packet {
	public int entityId;
	public byte effectId;
	public byte effectAmp;
	public short duration;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.entityId = var1.readInt();
		this.effectId = var1.readByte();
		this.effectAmp = var1.readByte();
		this.duration = var1.readShort();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.entityId);
		var1.writeByte(this.effectId);
		var1.writeByte(this.effectAmp);
		var1.writeShort(this.duration);
	}

	public void processPacket(NetHandler var1) {
		var1.handleEntityEffect(this);
	}

	public int getPacketSize() {
		return 8;
	}
}
