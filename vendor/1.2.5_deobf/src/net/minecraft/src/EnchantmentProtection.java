package net.minecraft.src;

public class EnchantmentProtection extends Enchantment {
	private static final String[] protectionName = new String[]{"all", "fire", "fall", "explosion", "projectile"};
	private static final int[] baseEnchantability = new int[]{1, 10, 5, 5, 3};
	private static final int[] levelEnchantability = new int[]{16, 8, 6, 8, 6};
	private static final int[] thresholdEnchantability = new int[]{20, 12, 10, 12, 15};
	public final int protectionType;

	public EnchantmentProtection(int var1, int var2, int var3) {
		super(var1, var2, EnumEnchantmentType.armor);
		this.protectionType = var3;
		if(var3 == 2) {
			this.type = EnumEnchantmentType.armor_feet;
		}

	}

	public int getMinEnchantability(int var1) {
		return baseEnchantability[this.protectionType] + (var1 - 1) * levelEnchantability[this.protectionType];
	}

	public int getMaxEnchantability(int var1) {
		return this.getMinEnchantability(var1) + thresholdEnchantability[this.protectionType];
	}

	public int getMaxLevel() {
		return 4;
	}

	public int calcModifierDamage(int var1, DamageSource var2) {
		if(var2.canHarmInCreative()) {
			return 0;
		} else {
			int var3 = (6 + var1 * var1) / 2;
			return this.protectionType == 0 ? var3 : (this.protectionType == 1 && var2.fireDamage() ? var3 : (this.protectionType == 2 && var2 == DamageSource.fall ? var3 * 2 : (this.protectionType == 3 && var2 == DamageSource.explosion ? var3 : (this.protectionType == 4 && var2.isProjectile() ? var3 : 0))));
		}
	}

	public String getName() {
		return "enchantment.protect." + protectionName[this.protectionType];
	}

	public boolean canApplyTogether(Enchantment var1) {
		if(var1 instanceof EnchantmentProtection) {
			EnchantmentProtection var2 = (EnchantmentProtection)var1;
			return var2.protectionType == this.protectionType ? false : this.protectionType == 2 || var2.protectionType == 2;
		} else {
			return super.canApplyTogether(var1);
		}
	}
}
