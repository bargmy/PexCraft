package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet103SetSlot extends Packet {
	public int windowId;
	public int itemSlot;
	public ItemStack myItemStack;

	public void processPacket(NetHandler var1) {
		var1.handleSetSlot(this);
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.windowId = var1.readByte();
		this.itemSlot = var1.readShort();
		this.myItemStack = this.readItemStack(var1);
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeByte(this.windowId);
		var1.writeShort(this.itemSlot);
		this.writeItemStack(this.myItemStack, var1);
	}

	public int getPacketSize() {
		return 8;
	}
}
