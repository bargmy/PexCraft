package net.minecraft.src;

public class EnchantmentDigging extends Enchantment {
	protected EnchantmentDigging(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.digger);
		this.setName("digging");
	}

	public int getMinEnchantability(int var1) {
		return 1 + 15 * (var1 - 1);
	}

	public int getMaxEnchantability(int var1) {
		return super.getMinEnchantability(var1) + 50;
	}

	public int getMaxLevel() {
		return 5;
	}
}
