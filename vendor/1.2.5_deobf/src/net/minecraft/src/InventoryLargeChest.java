package net.minecraft.src;

public class InventoryLargeChest implements IInventory {
	private String name;
	private IInventory upperChest;
	private IInventory lowerChest;

	public InventoryLargeChest(String var1, IInventory var2, IInventory var3) {
		this.name = var1;
		if(var2 == null) {
			var2 = var3;
		}

		if(var3 == null) {
			var3 = var2;
		}

		this.upperChest = var2;
		this.lowerChest = var3;
	}

	public int getSizeInventory() {
		return this.upperChest.getSizeInventory() + this.lowerChest.getSizeInventory();
	}

	public String getInvName() {
		return this.name;
	}

	public ItemStack getStackInSlot(int var1) {
		return var1 >= this.upperChest.getSizeInventory() ? this.lowerChest.getStackInSlot(var1 - this.upperChest.getSizeInventory()) : this.upperChest.getStackInSlot(var1);
	}

	public ItemStack decrStackSize(int var1, int var2) {
		return var1 >= this.upperChest.getSizeInventory() ? this.lowerChest.decrStackSize(var1 - this.upperChest.getSizeInventory(), var2) : this.upperChest.decrStackSize(var1, var2);
	}

	public ItemStack getStackInSlotOnClosing(int var1) {
		return var1 >= this.upperChest.getSizeInventory() ? this.lowerChest.getStackInSlotOnClosing(var1 - this.upperChest.getSizeInventory()) : this.upperChest.getStackInSlotOnClosing(var1);
	}

	public void setInventorySlotContents(int var1, ItemStack var2) {
		if(var1 >= this.upperChest.getSizeInventory()) {
			this.lowerChest.setInventorySlotContents(var1 - this.upperChest.getSizeInventory(), var2);
		} else {
			this.upperChest.setInventorySlotContents(var1, var2);
		}

	}

	public int getInventoryStackLimit() {
		return this.upperChest.getInventoryStackLimit();
	}

	public void onInventoryChanged() {
		this.upperChest.onInventoryChanged();
		this.lowerChest.onInventoryChanged();
	}

	public boolean isUseableByPlayer(EntityPlayer var1) {
		return this.upperChest.isUseableByPlayer(var1) && this.lowerChest.isUseableByPlayer(var1);
	}

	public void openChest() {
		this.upperChest.openChest();
		this.lowerChest.openChest();
	}

	public void closeChest() {
		this.upperChest.closeChest();
		this.lowerChest.closeChest();
	}
}
