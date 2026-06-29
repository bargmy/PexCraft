package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class NBTTagList extends NBTBase {
	private List tagList = new ArrayList();
	private byte tagType;

	public NBTTagList() {
		super("");
	}

	public NBTTagList(String var1) {
		super(var1);
	}

	void write(DataOutput var1) throws IOException {
		if(this.tagList.size() > 0) {
			this.tagType = ((NBTBase)this.tagList.get(0)).getId();
		} else {
			this.tagType = 1;
		}

		var1.writeByte(this.tagType);
		var1.writeInt(this.tagList.size());

		for(int var2 = 0; var2 < this.tagList.size(); ++var2) {
			((NBTBase)this.tagList.get(var2)).write(var1);
		}

	}

	void load(DataInput var1) throws IOException {
		this.tagType = var1.readByte();
		int var2 = var1.readInt();
		this.tagList = new ArrayList();

		for(int var3 = 0; var3 < var2; ++var3) {
			NBTBase var4 = NBTBase.newTag(this.tagType, (String)null);
			var4.load(var1);
			this.tagList.add(var4);
		}

	}

	public byte getId() {
		return (byte)9;
	}

	public String toString() {
		return "" + this.tagList.size() + " entries of type " + NBTBase.getTagName(this.tagType);
	}

	public void appendTag(NBTBase var1) {
		this.tagType = var1.getId();
		this.tagList.add(var1);
	}

	public NBTBase tagAt(int var1) {
		return (NBTBase)this.tagList.get(var1);
	}

	public int tagCount() {
		return this.tagList.size();
	}

	public NBTBase copy() {
		NBTTagList var1 = new NBTTagList(this.getName());
		var1.tagType = this.tagType;
		Iterator var2 = this.tagList.iterator();

		while(var2.hasNext()) {
			NBTBase var3 = (NBTBase)var2.next();
			NBTBase var4 = var3.copy();
			var1.tagList.add(var4);
		}

		return var1;
	}

	public boolean equals(Object var1) {
		if(super.equals(var1)) {
			NBTTagList var2 = (NBTTagList)var1;
			if(this.tagType == var2.tagType) {
				return this.tagList.equals(var2.tagList);
			}
		}

		return false;
	}

	public int hashCode() {
		return super.hashCode() ^ this.tagList.hashCode();
	}
}
