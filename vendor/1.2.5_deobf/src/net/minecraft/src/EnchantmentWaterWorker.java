package net.minecraft.src;

public class EnchantmentWaterWorker extends Enchantment {
	public EnchantmentWaterWorker(int var1, int var2) {
		super(var1, var2, EnumEnchantmentType.armor_head);
		this.setName("waterWorker");
	}

	public int getMinEnchantability(int var1) {
		return 1;
	}

	public int getMaxEnchantability(int var1) {
		return this.getMinEnchantability(var1) + 40;
	}

	public int getMaxLevel() {
		return 1;
	}
}
