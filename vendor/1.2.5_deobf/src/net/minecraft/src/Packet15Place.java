package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet15Place extends Packet {
	public int xPosition;
	public int yPosition;
	public int zPosition;
	public int direction;
	public ItemStack itemStack;

	public Packet15Place() {
	}

	public Packet15Place(int var1, int var2, int var3, int var4, ItemStack var5) {
		this.xPosition = var1;
		this.yPosition = var2;
		this.zPosition = var3;
		this.direction = var4;
		this.itemStack = var5;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.xPosition = var1.readInt();
		this.yPosition = var1.read();
		this.zPosition = var1.readInt();
		this.direction = var1.read();
		this.itemStack = this.readItemStack(var1);
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeInt(this.xPosition);
		var1.write(this.yPosition);
		var1.writeInt(this.zPosition);
		var1.write(this.direction);
		this.writeItemStack(this.itemStack, var1);
	}

	public void processPacket(NetHandler var1) {
		var1.handlePlace(this);
	}

	public int getPacketSize() {
		return 15;
	}
}
