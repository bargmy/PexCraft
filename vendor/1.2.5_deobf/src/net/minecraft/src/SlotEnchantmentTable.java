package net.minecraft.src;

class SlotEnchantmentTable extends InventoryBasic {
	final ContainerEnchantment container;

	SlotEnchantmentTable(ContainerEnchantment var1, String var2, int var3) {
		super(var2, var3);
		this.container = var1;
	}

	public int getInventoryStackLimit() {
		return 1;
	}

	public void onInventoryChanged() {
		super.onInventoryChanged();
		this.container.onCraftMatrixChanged(this);
	}
}
