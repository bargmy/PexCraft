package net.minecraft.src;

final class EnchantmentModifierLiving implements IEnchantmentModifier {
	public int livingModifier;
	public EntityLiving entityLiving;

	private EnchantmentModifierLiving() {
	}

	public void calculateModifier(Enchantment var1, int var2) {
		this.livingModifier += var1.calcModifierLiving(var2, this.entityLiving);
	}

	EnchantmentModifierLiving(Empty3 var1) {
		this();
	}
}
