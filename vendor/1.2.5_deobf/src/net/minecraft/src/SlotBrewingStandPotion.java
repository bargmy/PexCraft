package net.minecraft.src;

class SlotBrewingStandPotion extends Slot {
	private EntityPlayer player;
	final ContainerBrewingStand container;

	public SlotBrewingStandPotion(ContainerBrewingStand var1, EntityPlayer var2, IInventory var3, int var4, int var5, int var6) {
		super(var3, var4, var5, var6);
		this.container = var1;
		this.player = var2;
	}

	public boolean isItemValid(ItemStack var1) {
		return var1 != null && (var1.itemID == Item.potion.shiftedIndex || var1.itemID == Item.glassBottle.shiftedIndex);
	}

	public int getSlotStackLimit() {
		return 1;
	}

	public void onPickupFromSlot(ItemStack var1) {
		if(var1.itemID == Item.potion.shiftedIndex && var1.getItemDamage() > 0) {
			this.player.addStat(AchievementList.potion, 1);
		}

		super.onPickupFromSlot(var1);
	}
}
