package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagLong extends NBTBase {
	public long data;

	public NBTTagLong(String var1) {
		super(var1);
	}

	public NBTTagLong(String var1, long var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeLong(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readLong();
	}

	public byte getId() {
		return (byte)4;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagLong(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagLong var2 = (NBTTagLong)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ (int)(this.data ^ this.data >>> 32);
	}
}
