package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet43Experience extends Packet {
	public float experience;
	public int experienceTotal;
	public int experienceLevel;

	public void readPacketData(DataInputStream var1) throws IOException {
		this.experience = var1.readFloat();
		this.experienceLevel = var1.readShort();
		this.experienceTotal = var1.readShort();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeFloat(this.experience);
		var1.writeShort(this.experienceLevel);
		var1.writeShort(this.experienceTotal);
	}

	public void processPacket(NetHandler var1) {
		var1.handleExperience(this);
	}

	public int getPacketSize() {
		return 4;
	}
}
