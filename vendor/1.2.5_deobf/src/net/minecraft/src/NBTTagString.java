package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagString extends NBTBase {
	public String data;

	public NBTTagString(String var1) {
		super(var1);
	}

	public NBTTagString(String var1, String var2) {
		super(var1);
		this.data = var2;
		if(var2 == null) {
			throw new IllegalArgumentException("Empty string not allowed");
		}
	}

	void write(DataOutput var1) throws IOException {
		var1.writeUTF(this.data);
	}

	void load(DataInput var1) throws IOException {
		this.data = var1.readUTF();
	}

	public byte getId() {
		return (byte)8;
	}

	public String toString() {
		return "" + this.data;
	}

	public NBTBase copy() {
		return new NBTTagString(this.getName(), this.data);
	}

	public boolean equals(Object var1) {
		if(!super.equals(var1)) {
			return false;
		} else {
			NBTTagString var2 = (NBTTagString)var1;
			return this.data == null && var2.data == null || this.data != null && this.data.equals(var2.data);
		}
	}

	public int hashCode() {
		return super.hashCode() ^ this.data.hashCode();
	}
}
