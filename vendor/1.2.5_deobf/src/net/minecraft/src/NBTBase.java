package net.minecraft.src;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public abstract class NBTBase {
	private String name;

	abstract void write(DataOutput var1) throws IOException;

	abstract void load(DataInput var1) throws IOException;

	public abstract byte getId();

	protected NBTBase(String var1) {
		if(var1 == null) {
			this.name = "";
		} else {
			this.name = var1;
		}

	}

	public NBTBase setName(String var1) {
		if(var1 == null) {
			this.name = "";
		} else {
			this.name = var1;
		}

		return this;
	}

	public String getName() {
		return this.name == null ? "" : this.name;
	}

	public static NBTBase readNamedTag(DataInput var0) throws IOException {
		byte var1 = var0.readByte();
		if(var1 == 0) {
			return new NBTTagEnd();
		} else {
			String var2 = var0.readUTF();
			NBTBase var3 = newTag(var1, var2);
			var3.load(var0);
			return var3;
		}
	}

	public static void writeNamedTag(NBTBase var0, DataOutput var1) throws IOException {
		var1.writeByte(var0.getId());
		if(var0.getId() != 0) {
			var1.writeUTF(var0.getName());
			var0.write(var1);
		}
	}

	public static NBTBase newTag(byte var0, String var1) {
		switch(var0) {
		case 0:
			return new NBTTagEnd();
		case 1:
			return new NBTTagByte(var1);
		case 2:
			return new NBTTagShort(var1);
		case 3:
			return new NBTTagInt(var1);
		case 4:
			return new NBTTagLong(var1);
		case 5:
			return new NBTTagFloat(var1);
		case 6:
			return new NBTTagDouble(var1);
		case 7:
			return new NBTTagByteArray(var1);
		case 8:
			return new NBTTagString(var1);
		case 9:
			return new NBTTagList(var1);
		case 10:
			return new NBTTagCompound(var1);
		case 11:
			return new NBTTagIntArray(var1);
		default:
			return null;
		}
	}

	public static String getTagName(byte var0) {
		switch(var0) {
		case 0:
			return "TAG_End";
		case 1:
			return "TAG_Byte";
		case 2:
			return "TAG_Short";
		case 3:
			return "TAG_Int";
		case 4:
			return "TAG_Long";
		case 5:
			return "TAG_Float";
		case 6:
			return "TAG_Double";
		case 7:
			return "TAG_Byte_Array";
		case 8:
			return "TAG_String";
		case 9:
			return "TAG_List";
		case 10:
			return "TAG_Compound";
		case 11:
			return "TAG_Int_Array";
		default:
			return "UNKNOWN";
		}
	}

	public abstract NBTBase copy();

	public boolean equals(Object var1) {
		if(var1 != null && var1 instanceof NBTBase) {
			NBTBase var2 = (NBTBase)var1;
			return this.getId() != var2.getId() ? false : ((this.name != null || var2.name == null) && (this.name == null || var2.name != null) ? this.name == null || this.name.equals(var2.name) : false);
		} else {
			return false;
		}
	}

	public int hashCode() {
		return this.name.hashCode() ^ this.getId();
	}
}
