package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagShort extends NBTBase {
	public short data;

	public NBTTagShort(String var1) {
		super(var1);
	}

	public NBTTagShort(String var1, short var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeShort(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readShort();
	}

	public byte getId() {
		return (byte)2;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagShort(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagShort var2 = (NBTTagShort)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ this.data;
	}
}
