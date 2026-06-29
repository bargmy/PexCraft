package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet107CreativeSetSlot extends Packet {
	public int slot;
	public ItemStack itemStack;

	public Packet107CreativeSetSlot() {
	}

	public Packet107CreativeSetSlot(int var1, ItemStack var2) {
		this.slot = var1;
		this.itemStack = var2;
	}

	public void processPacket(NetHandler var1) {
		var1.handleCreativeSetSlot(this);
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.slot = var1.readShort();
		this.itemStack = this.readItemStack(var1);
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeShort(this.slot);
		this.writeItemStack(this.itemStack, var1);
	}

	public int getPacketSize() {
		return 8;
	}
}
