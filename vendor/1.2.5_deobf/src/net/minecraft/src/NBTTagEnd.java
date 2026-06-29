package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public class NBTTagEnd extends NBTBase {
	public NBTTagEnd() {
		super((String)null);
	}

	void load(DataInput var1) throws IOException {
	}

	void write(DataOutput var1) throws IOException {
	}

	public byte getId() {
		return (byte)0;
	}

	public String toString() {
		return "END";
	}

	public NBTBase copy() {
		return new NBTTagEnd();
	}

	public boolean equals(Object var1) {
		return super.equals(var1);
	}
}
