package net.minecraft.src;

public class EnchantmentArrowKnockback extends Enchantment {
	public EnchantmentArrowKnockback(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.bow);
		this.setName("arrowKnockback");
	}

	public int getMinEnchantability(int var1) {
		return 12 + (var1 - 1) * 20;
	}

	public int getMaxEnchantability(int var1) {
		return this.getMinEnchantability(var1) + 25;
	}

	public int getMaxLevel() {
		return 2;
	}
}
