package net.minecraft.src;

public class ItemMetadata extends ItemBlock {
	private Block blockObj;

	public ItemMetadata(int var1, Block var2) {
		super(var1);
		this.blockObj = var2;
		this.setMaxDamage(0);
		this.setHasSubtypes(true);
	}

	public int getIconFromDamage(int var1) {
		return this.blockObj.getBlockTextureFromSideAndMetadata(2, var1);
	}

	public int getMetadata(int var1) {
		return var1;
	}
}
