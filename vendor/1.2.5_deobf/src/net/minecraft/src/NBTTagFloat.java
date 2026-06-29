package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagFloat extends NBTBase {
	public float data;

	public NBTTagFloat(String var1) {
		super(var1);
	}

	public NBTTagFloat(String var1, float var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeFloat(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readFloat();
	}

	public byte getId() {
		return (byte)5;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagFloat(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagFloat var2 = (NBTTagFloat)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ Float.floatToIntBits(this.data);
	}
}
