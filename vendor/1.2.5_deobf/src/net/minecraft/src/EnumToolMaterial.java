package net.minecraft.src;

public enum EnumToolMaterial {
	WOOD(0, 59, 2.0F, 0, 15),
	STONE(1, 131, 4.0F, 1, 5),
	IRON(2, 250, 6.0F, 2, 14),
	EMERALD(3, 1561, 8.0F, 3, 10),
	GOLD(0, 32, 12.0F, 0, 22);

	private final int harvestLevel;
	private final int maxUses;
	private final float efficiencyOnProperMaterial;
	private final int damageVsEntity;
	private final int enchantability;

	private EnumToolMaterial(int var3, int var4, float var5, int var6, int var7) {
		this.harvestLevel = var3;
		this.maxUses = var4;
		this.efficiencyOnProperMaterial = var5;
		this.damageVsEntity = var6;
		this.enchantability = var7;
	}

	public int getMaxUses() {
		return this.maxUses;
	}

	public float getEfficiencyOnProperMaterial() {
		return this.efficiencyOnProperMaterial;
	}

	public int getDamageVsEntity() {
		return this.damageVsEntity;
	}

	public int getHarvestLevel() {
		return this.harvestLevel;
	}

	public int getEnchantability() {
		return this.enchantability;
	}
}
