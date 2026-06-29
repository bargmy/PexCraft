package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet108EnchantItem extends Packet {
	public int windowId;
	public int enchantment;

	public Packet108EnchantItem() {
	}

	public Packet108EnchantItem(int var1, int var2) {
		this.windowId = var1;
		this.enchantment = var2;
	}

	public void processPacket(NetHandler var1) {
		var1.handleEnchantItem(this);
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.windowId = var1.readByte();
		this.enchantment = var1.readByte();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeByte(this.windowId);
		var1.writeByte(this.enchantment);
	}

	public int getPacketSize() {
		return 2;
	}
}
