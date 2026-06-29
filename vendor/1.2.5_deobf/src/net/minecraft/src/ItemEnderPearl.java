package net.minecraft.src;

public class ItemEnderPearl extends Item {
	public ItemEnderPearl(int var1) {
		super(var1);
		this.maxStackSize = 16;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		if(var3.capabilities.isCreativeMode) {
			return var1;
		} else if(var3.ridingEntity != null) {
			return var1;
		} else {
			--var1.stackSize;
			var2.playSoundAtEntity(var3, "random.bow", 0.5F, 0.4F / (itemRand.nextFloat() * 0.4F + 0.8F));
			if(!var2.isRemote) {
				var2.spawnEntityInWorld(new EntityEnderPearl(var2, var3));
			}

			return var1;
		}
	}
}
