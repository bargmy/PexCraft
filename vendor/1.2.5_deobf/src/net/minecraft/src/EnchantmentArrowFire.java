package net.minecraft.src;

public class EnchantmentArrowFire extends Enchantment {
	public EnchantmentArrowFire(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.bow);
		this.setName("arrowFire");
	}

	public int getMinEnchantability(int var1) {
		return 20;
	}

	public int getMaxEnchantability(int var1) {
		return 50;
	}

	public int getMaxLevel() {
		return 1;
	}
}
