package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.Arrays;

public class NBTTagIntArray extends NBTBase {
	public int[] field_48181_a;

	public NBTTagIntArray(String var1) {
		super(var1);
	}

	public NBTTagIntArray(String var1, int[] var2) {
		super(var1);
		this.field_48181_a = var2;
	}

	void write(DataOutput var1) throws IOException {
		var1.writeInt(this.field_48181_a.length);

		for(int var2 = 0; var2 < this.field_48181_a.length; ++var2) {
			var1.writeInt(this.field_48181_a[var2]);
		}

	}

	void load(DataInput var1) throws IOException {
		int var2 = var1.readInt();
		this.field_48181_a = new int[var2];

		for(int var3 = 0; var3 < var2; ++var3) {
			this.field_48181_a[var3] = var1.readInt();
		}

	}

	public byte getId() {
		return (byte)11;
	}

	public String toString() {
		return "[" + this.field_48181_a.length + " bytes]";
	}

	public NBTBase copy() {
		int[] var1 = new int[this.field_48181_a.length];
		System.arraycopy(this.field_48181_a, 0, var1, 0, this.field_48181_a.length);
		return new NBTTagIntArray(this.getName(), var1);
	}

	public boolean equals(Object var1) {
		if(!super.equals(var1)) {
			return false;
		} else {
			NBTTagIntArray var2 = (NBTTagIntArray)var1;
			return this.field_48181_a == null && var2.field_48181_a == null || this.field_48181_a != null && this.field_48181_a.equals(var2.field_48181_a);
		}
	}

	public int hashCode() {
		return super.hashCode() ^ Arrays.hashCode(this.field_48181_a);
	}
}
