package net.minecraft.src;

class SlotBrewingStandIngredient extends Slot {
	final ContainerBrewingStand container;

	public SlotBrewingStandIngredient(ContainerBrewingStand var1, IInventory var2, int var3, int var4, int var5) {
		super(var2, var3, var4, var5);
		this.container = var1;
	}

	public boolean isItemValid(ItemStack var1) {
		return var1 != null ? Item.itemsList[var1.itemID].isPotionIngredient() : false;
	}

	public int getSlotStackLimit() {
		return 64;
	}
}
