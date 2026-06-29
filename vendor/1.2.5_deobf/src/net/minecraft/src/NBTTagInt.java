package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagInt extends NBTBase {
	public int data;

	public NBTTagInt(String var1) {
		super(var1);
	}

	public NBTTagInt(String var1, int var2) {
		super(var1);
		this.data = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeInt(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readInt();
	}

	public byte getId() {
		return (byte)3;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagInt(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagInt var2 = (NBTTagInt)var1;
			return this.data == var2.data;
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ this.data;
	}
}
