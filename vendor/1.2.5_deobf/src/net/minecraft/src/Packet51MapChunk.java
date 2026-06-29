package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

public class Packet51MapChunk extends Packet {
	public int xCh;
	public int zCh;
	public int yChMin;
	public int yChMax;
	public byte[] chunkData;
	public boolean includeInitialize;
	private int tempLength;
	private int field_48178_h;
	private static byte[] temp = new byte[0];

	public Packet51MapChunk() {
		this.isChunkDataPacket = true;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.xCh = var1.readInt();
		this.zCh = var1.readInt();
		this.includeInitialize = var1.readBoolean();
		this.yChMin = var1.readShort();
		this.yChMax = var1.readShort();
		this.tempLength = var1.readInt();
		this.field_48178_h = var1.readInt();
		if(temp.length < this.tempLength) {
			temp = new byte[this.tempLength];
		}

		var1.readFully(temp, 0, this.tempLength);
		int var2 = 0;

		int var3;
		for(var3 = 0; var3 < 16; ++var3) {
			var2 += this.yChMin >> var3 & 1;
		}

		var3 = 12288 * var2;
		if(this.includeInitialize) {
			var3 += 256;
		}

		this.chunkData = new byte[var3];
		Inflater var4 = new Inflater();
		var4.setInput(temp, 0, this.tempLength);

		try {
			var4.inflate(this.chunkData);
		} catch (DataFormatException var9) {
			throw new IOException("Bad compressed data format");
		} finally {
			var4.end();
		}

	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.xCh);
		var1.writeInt(this.zCh);
		var1.writeBoolean(this.includeInitialize);
		var1.writeShort((short)(this.yChMin & '\uffff'));
		var1.writeShort((short)(this.yChMax & '\uffff'));
		var1.writeInt(this.tempLength);
		var1.writeInt(this.field_48178_h);
		var1.write(this.chunkData, 0, this.tempLength);
	}

	public void processPacket(NetHandler var1) {
		var1.func_48487_a(this);
	}

	public int getPacketSize() {
		return 17 + this.tempLength;
	}
}
