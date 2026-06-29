package net.minecraft.src;

public class ServerNBTStorage {
	public String name;
	public String host;
	public String playerCount;
	public String motd;
	public long lag;
	public boolean polled = false;

	public ServerNBTStorage(String var1, String var2) {
		this.name = var1;
		this.host = var2;
	}

	public NBTTagCompound getCompoundTag() {
		NBTTagCompound var1 = new NBTTagCompound();
		var1.setString("name", this.name);
		var1.setString("ip", this.host);
		return var1;
	}

	public static ServerNBTStorage createServerNBTStorage(NBTTagCompound var0) {
		return new ServerNBTStorage(var0.getString("name"), var0.getString("ip"));
	}
}
