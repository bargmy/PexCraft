package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet132TileEntityData extends Packet {
	public int xPosition;
	public int yPosition;
	public int zPosition;
	public int actionType;
	public int customParam1;
	public int customParam2;
	public int customParam3;

	public Packet132TileEntityData() {
		this.isChunkDataPacket = true;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.xPosition = var1.readInt();
		this.yPosition = var1.readShort();
		this.zPosition = var1.readInt();
		this.actionType = var1.readByte();
		this.customParam1 = var1.readInt();
		this.customParam2 = var1.readInt();
		this.customParam3 = var1.readInt();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.xPosition);
		var1.writeShort(this.yPosition);
		var1.writeInt(this.zPosition);
		var1.writeByte((byte)this.actionType);
		var1.writeInt(this.customParam1);
		var1.writeInt(this.customParam2);
		var1.writeInt(this.customParam3);
	}

	public void processPacket(NetHandler var1) {
		var1.handleTileEntityData(this);
	}

	public int getPacketSize() {
		return 25;
	}
}
