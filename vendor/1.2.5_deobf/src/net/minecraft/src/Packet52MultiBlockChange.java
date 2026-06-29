package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet52MultiBlockChange extends Packet {
	public int xPosition;
	public int zPosition;
	public byte[] metadataArray;
	public int size;
	private static byte[] field_48168_e = new byte[0];

	public Packet52MultiBlockChange() {
		this.isChunkDataPacket = true;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.xPosition = var1.readInt();
		this.zPosition = var1.readInt();
		this.size = var1.readShort() & '\uffff';
		int var2 = var1.readInt();
		if(var2 > 0) {
			this.metadataArray = new byte[var2];
			var1.readFully(this.metadataArray);
		}

	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.xPosition);
		var1.writeInt(this.zPosition);
		var1.writeShort((short)this.size);
		if(this.metadataArray != null) {
			var1.writeInt(this.metadataArray.length);
			var1.write(this.metadataArray);
		} else {
			var1.writeInt(0);
		}

	}

	public void processPacket(NetHandler var1) {
		var1.handleMultiBlockChange(this);
	}

	public int getPacketSize() {
		return 10 + this.size * 4;
	}
}
