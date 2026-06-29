package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagDouble extends NBTBase {
	public double data;

	public NBTTagDouble(String var1) {
		super(var1);
	}

	public NBTTagDouble(String var1, double var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeDouble(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readDouble();
	}

	public byte getId() {
		return (byte)6;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagDouble(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagDouble var2 = (NBTTagDouble)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		long var1 = Double.doubleToLongBits(this.data);
		return super.hashCode() ^ (int)(var1 ^ var1 >>> 32);
	}
}
