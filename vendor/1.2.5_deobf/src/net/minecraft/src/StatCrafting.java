package net.minecraft.src;

public class StatCrafting extends StatBase {
	private final int itemID;

	public StatCrafting(int var1, String var2, int var3) {
		super(var1, var2);
		this.itemID = var3;
	}

	public int getItemID() {
		return this.itemID;
	}
}
