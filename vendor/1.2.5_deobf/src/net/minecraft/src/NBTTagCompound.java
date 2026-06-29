package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class NBTTagCompound extends NBTBase {
	private Map tagMap = new HashMap();

	public NBTTagCompound() {
		super("");
	}

	public NBTTagCompound(String var1) {
		super(var1);
	}

	void write(DataOutput var1) throws IOException {
		Iterator var2 = this.tagMap.values().iterator();

		while(var2.hasNext()) {
			NBTBase var3 = (NBTBase)var2.next();
			NBTBase.writeNamedTag(var3, var1);
		}

		var1.writeByte(0);
	}

	void load(DataInput var1) throws IOException {
		this.tagMap.clear();

		while(true) {
			NBTBase var2 = NBTBase.readNamedTag(var1);
			if(var2.getId() == 0) {
				return;
			}

			this.tagMap.put(var2.getName(), var2);
		}
	}

	public Collection getTags() {
		return this.tagMap.values();
	}

	public byte getId() {
		return (byte)10;
	}

	public void setTag(String var1, NBTBase var2) {
		this.tagMap.put(var1, var2.setName(var1));
	}

	public void setByte(String var1, byte var2) {
		this.tagMap.put(var1, new NBTTagByte(var1, var2));
	}

	public void setShort(String var1, short var2) {
		this.tagMap.put(var1, new NBTTagShort(var1, var2));
	}

	public void setInteger(String var1, int var2) {
		this.tagMap.put(var1, new NBTTagInt(var1, var2));
	}

	public void setLong(String var1, long var2) {
		this.tagMap.put(var1, new NBTTagLong(var1, var2));
	}

	public void setFloat(String var1, float var2) {
		this.tagMap.put(var1, new NBTTagFloat(var1, var2));
	}

	public void setDouble(String var1, double var2) {
		this.tagMap.put(var1, new NBTTagDouble(var1, var2));
	}

	public void setString(String var1, String var2) {
		this.tagMap.put(var1, new NBTTagString(var1, var2));
	}

	public void setByteArray(String var1, byte[] var2) {
		this.tagMap.put(var1, new NBTTagByteArray(var1, var2));
	}

	public void func_48183_a(String var1, int[] var2) {
		this.tagMap.put(var1, new NBTTagIntArray(var1, var2));
	}

	public void setCompoundTag(String var1, NBTTagCompound var2) {
		this.tagMap.put(var1, var2.setName(var1));
	}

	public void setBoolean(String var1, boolean var2) {
		this.setByte(var1, (byte)(var2 ? 1 : 0));
	}

	public NBTBase getTag(String var1) {
		return (NBTBase)this.tagMap.get(var1);
	}

	public boolean hasKey(String var1) {
		return this.tagMap.containsKey(var1);
	}

	public byte getByte(String var1) {
		return !this.tagMap.containsKey(var1) ? 0 : ((NBTTagByte)this.tagMap.get(var1)).data;
	}

	public short getShort(String var1) {
		return !this.tagMap.containsKey(var1) ? 0 : ((NBTTagShort)this.tagMap.get(var1)).data;
	}

	public int getInteger(String var1) {
		return !this.tagMap.containsKey(var1) ? 0 : ((NBTTagInt)this.tagMap.get(var1)).data;
	}

	public long getLong(String var1) {
		return !this.tagMap.containsKey(var1) ? 0L : ((NBTTagLong)this.tagMap.get(var1)).data;
	}

	public float getFloat(String var1) {
		return !this.tagMap.containsKey(var1) ? 0.0F : ((NBTTagFloat)this.tagMap.get(var1)).data;
	}

	public double getDouble(String var1) {
		return !this.tagMap.containsKey(var1) ? 0.0D : ((NBTTagDouble)this.tagMap.get(var1)).data;
	}

	public String getString(String var1) {
		return !this.tagMap.containsKey(var1) ? "" : ((NBTTagString)this.tagMap.get(var1)).data;
	}

	public byte[] getByteArray(String var1) {
		return !this.tagMap.containsKey(var1) ? new byte[0] : ((NBTTagByteArray)this.tagMap.get(var1)).byteArray;
	}

	public int[] func_48182_l(String var1) {
		return !this.tagMap.containsKey(var1) ? new int[0] : ((NBTTagIntArray)this.tagMap.get(var1)).field_48181_a;
	}

	public NBTTagCompound getCompoundTag(String var1) {
		return !this.tagMap.containsKey(var1) ? new NBTTagCompound(var1) : (NBTTagCompound)this.tagMap.get(var1);
	}

	public NBTTagList getTagList(String var1) {
		return !this.tagMap.containsKey(var1) ? new NBTTagList(var1) : (NBTTagList)this.tagMap.get(var1);
	}

	public boolean getBoolean(String var1) {
		return this.getByte(var1) != 0;
	}

	public String toString() {
		return "" + this.tagMap.size() + " entries";
	}

	public NBTBase copy() {
		NBTTagCompound var1 = new NBTTagCompound(this.getName());
		Iterator var2 = this.tagMap.keySet().iterator();

		while(var2.hasNext()) {
			String var3 = (String)var2.next();
			var1.setTag(var3, ((NBTBase)this.tagMap.get(var3)).copy());
		}

		return var1;
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagCompound var2 = (NBTTagCompound)var1;
			return this.tagMap.entrySet().equals(var2.tagMap.entrySet());
		} else {
			return false;
		}
	}

	public int hashCode() {
		return super.hashCode() ^ this.tagMap.hashCode();
	}
}
