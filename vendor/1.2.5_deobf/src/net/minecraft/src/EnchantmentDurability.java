package net.minecraft.src;

public class EnchantmentDurability extends Enchantment {
	protected EnchantmentDurability(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.digger);
		this.setName("durability");
	}

	public int getMinEnchantability(int var1) {
		return 5 + (var1 - 1) * 10;
	}

	public int getMaxEnchantability(int var1) {
		return super.getMinEnchantability(var1) + 50;
	}

	public int getMaxLevel() {
		return 3;
	}
}
