package net.minecraft.src;

public class ItemArmor extends Item {
	private static final int[] maxDamageArray = new int[]{11, 16, 15, 13};
	public final int armorType;
	public final int damageReduceAmount;
	public final int renderIndex;
	private final EnumArmorMaterial material;

	public ItemArmor(int var1, EnumArmorMaterial var2, int var3, int var4) {
		super(var1);
		this.material = var2;
		this.armorType = var4;
		this.renderIndex = var3;
		this.damageReduceAmount = var2.getDamageReductionAmount(var4);
		this.setMaxDamage(var2.getDurability(var4));
		this.maxStackSize = 1;
	}

	public int getItemEnchantability() {
		return this.material.getEnchantability();
	}

	static int[] getMaxDamageArray() {
		return maxDamageArray;
	}
}
