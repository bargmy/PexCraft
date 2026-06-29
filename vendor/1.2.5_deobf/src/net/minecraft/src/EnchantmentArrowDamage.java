package net.minecraft.src;

public class EnchantmentArrowDamage extends Enchantment {
	public EnchantmentArrowDamage(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.bow);
		this.setName("arrowDamage");
	}

	public int getMinEnchantability(int var1) {
		return 1 + (var1 - 1) * 10;
	}

	public int getMaxEnchantability(int var1) {
		return this.getMinEnchantability(var1) + 15;
	}

	public int getMaxLevel() {
		return 5;
	}
}
