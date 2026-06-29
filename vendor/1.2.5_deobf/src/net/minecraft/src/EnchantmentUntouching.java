package net.minecraft.src;

public class EnchantmentUntouching extends Enchantment {
	protected EnchantmentUntouching(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.digger);
		this.setName("untouching");
	}

	public int getMinEnchantability(int var1) {
		return 25;
	}

	public int getMaxEnchantability(int var1) {
		return super.getMinEnchantability(var1) + 50;
	}

	public int getMaxLevel() {
		return 1;
	}

	public boolean canApplyTogether(Enchantment var1) {
		return super.canApplyTogether(var1) && var1.effectId != fortune.effectId;
	}
}
