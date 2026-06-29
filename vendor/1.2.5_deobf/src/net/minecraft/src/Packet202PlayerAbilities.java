package net.minecraft.src;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Packet202PlayerAbilities extends Packet {
	public boolean field_50072_a = false;
	public boolean field_50070_b = false;
	public boolean field_50071_c = false;
	public boolean field_50069_d = false;

	public Packet202PlayerAbilities() {
	}

	public Packet202PlayerAbilities(PlayerCapabilities var1) {
		this.field_50072_a = var1.disableDamage;
		this.field_50070_b = var1.isFlying;
		this.field_50071_c = var1.allowFlying;
		this.field_50069_d = var1.isCreativeMode;
	}

	public void readPacketData(DataInputStream var1) throws IOException {
		this.field_50072_a = var1.readBoolean();
		this.field_50070_b = var1.readBoolean();
		this.field_50071_c = var1.readBoolean();
		this.field_50069_d = var1.readBoolean();
	}

	public void writePacketData(DataOutputStream var1) throws IOException {
		var1.writeBoolean(this.field_50072_a);
		var1.writeBoolean(this.field_50070_b);
		var1.writeBoolean(this.field_50071_c);
		var1.writeBoolean(this.field_50069_d);
	}

	public void processPacket(NetHandler var1) {
		var1.func_50100_a(this);
	}

	public int getPacketSize() {
		return 1;
	}
}
