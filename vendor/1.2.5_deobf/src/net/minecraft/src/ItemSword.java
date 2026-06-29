package net.minecraft.src;

public class ItemSword extends Item {
	private int weaponDamage;
	private final EnumToolMaterial toolMaterial;

	public ItemSword(int var1, EnumToolMaterial var2) {
		super(var1);
		this.toolMaterial = var2;
		this.maxStackSize = 1;
		this.setMaxDamage(var2.getMaxUses());
		this.weaponDamage = 4 + var2.getDamageVsEntity();
	}

	public float getStrVsBlock(ItemStack var1, Block var2) {
		return var2.blockID == Block.web.blockID ? 15.0F : 1.5F;
	}

	public boolean hitEntity(ItemStack var1, EntityLiving var2, EntityLiving var3) {
		var1.damageItem(1, var3);
		return true;
	}

	public boolean onBlockDestroyed(ItemStack var1, int var2, int var3, int var4, int var5, EntityLiving var6) {
		var1.damageItem(2, var6);
		return true;
	}

	public int getDamageVsEntity(Entity var1) {
		return this.weaponDamage;
	}

	public boolean isFull3D() {
		return true;
	}

	public EnumAction getItemUseAction(ItemStack var1) {
		return EnumAction.block;
	}

	public int getMaxItemUseDuration(ItemStack var1) {
		return 72000;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		var3.setItemInUse(var1, this.getMaxItemUseDuration(var1));
		return var1;
	}

	public boolean canHarvestBlock(Block var1) {
		return var1.blockID == Block.web.blockID;
	}

	public int getItemEnchantability() {
		return this.toolMaterial.getEnchantability();
	}
}
