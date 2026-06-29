package net.minecraft.src;

final class EnchantmentModifierDamage implements IEnchantmentModifier {
	public int damageModifier;
	public DamageSource damageSource;

	private EnchantmentModifierDamage() {
	}

	public void calculateModifier(Enchantment var1, int var2) {
		this.damageModifier += var1.calcModifierDamage(var2, this.damageSource);
	}

	EnchantmentModifierDamage(Empty3 var1) {
		this();
	}
}
