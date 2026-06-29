package net.minecraft.src;

public class ItemColored extends ItemBlock {
	private final Block blockRef = Block.blocksList[this.getBlockID()];
	private String[] blockNames;

	public ItemColored(int var1, boolean var2) {
		super(var1);
		if(var2) {
			this.setMaxDamage(0);
			this.setHasSubtypes(true);
		}

	}

	public int getColorFromDamage(int var1, int var2) {
		return this.blockRef.getRenderColor(var1);
	}

	public int getIconFromDamage(int var1) {
		return this.blockRef.getBlockTextureFromSideAndMetadata(0, var1);
	}

	public int getMetadata(int var1) {
		return var1;
	}

	public ItemColored setBlockNames(String[] var1) {
		this.blockNames = var1;
		return this;
	}

	public String getItemNameIS(ItemStack var1) {
		if(this.blockNames == null) {
			return super.getItemNameIS(var1);
		} else {
			int var2 = var1.getItemDamage();
			return var2 >= 0 && var2 < this.blockNames.length ? super.getItemNameIS(var1) + "." + this.blockNames[var2] : super.getItemNameIS(var1);
		}
	}
}
