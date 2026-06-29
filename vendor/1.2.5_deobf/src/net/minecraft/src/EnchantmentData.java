package net.minecraft.src;

public class EnchantmentData extends WeightedRandomChoice {
	public final Enchantment enchantmentobj;
	public final int enchantmentLevel;

	public EnchantmentData(Enchantment var1, int var2) {
		super(var1.getWeight());
		this.enchantmentobj = var1;
		this.enchantmentLevel = var2;
	}
}
