package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagByte extends NBTBase {
	public byte data;

	public NBTTagByte(String var1) {
		super(var1);
	}

	public NBTTagByte(String var1, byte var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeByte(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readByte();
	}

	public byte getId() {
		return (byte)1;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagByte(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagByte var2 = (NBTTagByte)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ this.data;
	}
}
