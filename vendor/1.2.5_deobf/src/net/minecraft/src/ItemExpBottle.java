package net.minecraft.src;

public class ItemExpBottle extends Item {
	public ItemExpBottle(int var1) {
		super(var1);
	}

	public boolean hasEffect(ItemStack var1) {
		return true;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		if(!var3.capabilities.isCreativeMode) {
			--var1.stackSize;
		}

		var2.playSoundAtEntity(var3, "random.bow", 0.5F, 0.4F / (itemRand.nextFloat() * 0.4F + 0.8F));
		if(!var2.isRemote) {
			var2.spawnEntityInWorld(new EntityExpBottle(var2, var3));
		}

		return var1;
	}
}
