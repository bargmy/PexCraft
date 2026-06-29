package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet35EntityHeadRotation extends Packet {
	public int entityId;
	public byte headRotationYaw;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.entityId = var1.readInt();
		this.headRotationYaw = var1.readByte();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.entityId);
		var1.writeByte(this.headRotationYaw);
	}

	public void processPacket(NetHandler var1) {
		var1.handleEntityHeadRotation(this);
	}

	public int getPacketSize() {
		return 5;
	}
}
