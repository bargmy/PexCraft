package net.minecraft.src;

public class SlotFurnace extends Slot {
	private EntityPlayer thePlayer;
	private int field_48437_f;

	public SlotFurnace(EntityPlayer var1, IInventory var2, int var3, int var4, int var5) {
		super(var2, var3, var4, var5);
		this.thePlayer = var1;
	}

	public boolean isItemValid(ItemStack var1) {
		return false;
	}

	public ItemStack decrStackSize(int var1) {
		if(this.getHasStack()) {
			this.field_48437_f += Math.min(var1, this.getStack().stackSize);
		}

		return super.decrStackSize(var1);
	}

	public void onPickupFromSlot(ItemStack var1) {
		this.func_48434_c(var1);
		super.onPickupFromSlot(var1);
	}

	protected void func_48435_a(ItemStack var1, int var2) {
		this.field_48437_f += var2;
		this.func_48434_c(var1);
	}

	protected void func_48434_c(ItemStack var1) {
		var1.onCrafting(this.thePlayer.worldObj, this.thePlayer, this.field_48437_f);
		this.field_48437_f = 0;
		if(var1.itemID == Item.ingotIron.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.acquireIron, 1);
		}

		if(var1.itemID == Item.fishCooked.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.cookFish, 1);
		}

	}
}
