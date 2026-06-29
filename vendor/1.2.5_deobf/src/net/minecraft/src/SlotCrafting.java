package net.minecraft.src;

public class SlotCrafting extends Slot {
	private final IInventory craftMatrix;
	private EntityPlayer thePlayer;
	private int field_48436_g;

	public SlotCrafting(EntityPlayer var1, IInventory var2, IInventory var3, int var4, int var5, int var6) {
		super(var3, var4, var5, var6);
		this.thePlayer = var1;
		this.craftMatrix = var2;
	}

	public boolean isItemValid(ItemStack var1) {
		return false;
	}

	public ItemStack decrStackSize(int var1) {
		if(this.getHasStack()) {
			this.field_48436_g += Math.min(var1, this.getStack().stackSize);
		}

		return super.decrStackSize(var1);
	}

	protected void func_48435_a(ItemStack var1, int var2) {
		this.field_48436_g += var2;
		this.func_48434_c(var1);
	}

	protected void func_48434_c(ItemStack var1) {
		var1.onCrafting(this.thePlayer.worldObj, this.thePlayer, this.field_48436_g);
		this.field_48436_g = 0;
		if(var1.itemID == Block.workbench.blockID) {
			this.thePlayer.addStat(AchievementList.buildWorkBench, 1);
		} else if(var1.itemID == Item.pickaxeWood.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.buildPickaxe, 1);
		} else if(var1.itemID == Block.stoneOvenIdle.blockID) {
			this.thePlayer.addStat(AchievementList.buildFurnace, 1);
		} else if(var1.itemID == Item.hoeWood.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.buildHoe, 1);
		} else if(var1.itemID == Item.bread.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.makeBread, 1);
		} else if(var1.itemID == Item.cake.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.bakeCake, 1);
		} else if(var1.itemID == Item.pickaxeStone.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.buildBetterPickaxe, 1);
		} else if(var1.itemID == Item.swordWood.shiftedIndex) {
			this.thePlayer.addStat(AchievementList.buildSword, 1);
		} else if(var1.itemID == Block.enchantmentTable.blockID) {
			this.thePlayer.addStat(AchievementList.enchantments, 1);
		} else if(var1.itemID == Block.bookShelf.blockID) {
			this.thePlayer.addStat(AchievementList.bookcase, 1);
		}

	}

	public void onPickupFromSlot(ItemStack var1) {
		this.func_48434_c(var1);

		for(int var2 = 0; var2 < this.craftMatrix.getSizeInventory(); ++var2) {
			ItemStack var3 = this.craftMatrix.getStackInSlot(var2);
			if(var3 != null) {
				this.craftMatrix.decrStackSize(var2, 1);
				if(var3.getItem().hasContainerItem()) {
					ItemStack var4 = new ItemStack(var3.getItem().getContainerItem());
					if(!var3.getItem().doesContainerItemLeaveCraftingGrid(var3) || !this.thePlayer.inventory.addItemStackToInventory(var4)) {
						if(this.craftMatrix.getStackInSlot(var2) == null) {
							this.craftMatrix.setInventorySlotContents(var2, var4);
						} else {
							this.thePlayer.dropPlayerItem(var4);
						}
					}
				}
			}
		}

	}
}
