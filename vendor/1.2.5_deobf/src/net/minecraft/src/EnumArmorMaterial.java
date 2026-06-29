package net.minecraft.src;

public enum EnumArmorMaterial {
	CLOTH(5, new int[]{1, 3, 2, 1}, 15),
	CHAIN(15, new int[]{2, 5, 4, 1}, 12),
	IRON(15, new int[]{2, 6, 5, 2}, 9),
	GOLD(7, new int[]{2, 5, 3, 1}, 25),
	DIAMOND(33, new int[]{3, 8, 6, 3}, 10);

	private int maxDamageFactor;
	private int[] damageReductionAmountArray;
	private int enchantability;

	private EnumArmorMaterial(int var3, int[] var4, int var5) {
		this.maxDamageFactor = var3;
		this.damageReductionAmountArray = var4;
		this.enchantability = var5;
	}

	public int getDurability(int var1) {
		return ItemArmor.getMaxDamageArray()[var1] * this.maxDamageFactor;
	}

	public int getDamageReductionAmount(int var1) {
		return this.damageReductionAmountArray[var1];
	}

	public int getEnchantability() {
		return this.enchantability;
	}
}
