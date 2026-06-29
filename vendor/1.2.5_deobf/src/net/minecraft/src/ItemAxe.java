package net.minecraft.src;

public class ItemAxe extends ItemTool {
	private static Block[] blocksEffectiveAgainst = new Block[]{Block.planks, Block.bookShelf, Block.wood, Block.chest, Block.stairDouble, Block.stairSingle, Block.pumpkin, Block.pumpkinLantern};

	protected ItemAxe(int var1, EnumToolMaterial var2) {
		super(var1, 3, var2, blocksEffectiveAgainst);
	}

	public float getStrVsBlock(ItemStack var1, Block var2) {
		return var2 != null && var2.blockMaterial == Material.wood ? this.efficiencyOnProperMaterial : super.getStrVsBlock(var1, var2);
	}
}
