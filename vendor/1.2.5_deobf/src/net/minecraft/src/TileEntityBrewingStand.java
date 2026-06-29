package net.minecraft.src;

import java.util.List;

public class TileEntityBrewingStand extends TileEntity implements IInventory {
	private ItemStack[] brewingItemStacks = new ItemStack[4];
	private int brewTime;
	private int filledSlots;
	private int ingredientID;

	public String getInvName() {
		return "container.brewing";
	}

	public int getSizeInventory() {
		return this.brewingItemStacks.length;
	}

	public void updateEntity() {
		if(this.brewTime > 0) {
			--this.brewTime;
			if(this.brewTime == 0) {
				this.brewPotions();
				this.onInventoryChanged();
			} else if(!this.canBrew()) {
				this.brewTime = 0;
				this.onInventoryChanged();
			} else if(this.ingredientID != this.brewingItemStacks[3].itemID) {
				this.brewTime = 0;
				this.onInventoryChanged();
			}
		} else if(this.canBrew()) {
			this.brewTime = 400;
			this.ingredientID = this.brewingItemStacks[3].itemID;
		}

		int var1 = this.getFilledSlots();
		if(var1 != this.filledSlots) {
			this.filledSlots = var1;
			this.worldObj.setBlockMetadataWithNotify(this.xCoord, this.yCoord, this.zCoord, var1);
		}

		super.updateEntity();
	}

	public int getBrewTime() {
		return this.brewTime;
	}

	private boolean canBrew() {
		if(this.brewingItemStacks[3] != null && this.brewingItemStacks[3].stackSize > 0) {
			ItemStack var1 = this.brewingItemStacks[3];
			if(!Item.itemsList[var1.itemID].isPotionIngredient()) {
				return false;
			} else {
				boolean var2 = false;

				for(int var3 = 0; var3 < 3; ++var3) {
					if(this.brewingItemStacks[var3] != null && this.brewingItemStacks[var3].itemID == Item.potion.shiftedIndex) {
						int var4 = this.brewingItemStacks[var3].getItemDamage();
						int var5 = this.getPotionResult(var4, var1);
						if(!ItemPotion.isSplash(var4) && ItemPotion.isSplash(var5)) {
							var2 = true;
							break;
						}

						List var6 = Item.potion.getEffects(var4);
						List var7 = Item.potion.getEffects(var5);
						if((var4 <= 0 || var6 != var7) && (var6 == null || !var6.equals(var7) && var7 != null) && var4 != var5) {
							var2 = true;
							break;
						}
					}
				}

				return var2;
			}
		} else {
			return false;
		}
	}

	private void brewPotions() {
		if(this.canBrew()) {
			ItemStack var1 = this.brewingItemStacks[3];

			for(int var2 = 0; var2 < 3; ++var2) {
				if(this.brewingItemStacks[var2] != null && this.brewingItemStacks[var2].itemID == Item.potion.shiftedIndex) {
					int var3 = this.brewingItemStacks[var2].getItemDamage();
					int var4 = this.getPotionResult(var3, var1);
					List var5 = Item.potion.getEffects(var3);
					List var6 = Item.potion.getEffects(var4);
					if(var3 > 0 && var5 == var6 || var5 != null && (var5.equals(var6) || var6 == null)) {
						if(!ItemPotion.isSplash(var3) && ItemPotion.isSplash(var4)) {
							this.brewingItemStacks[var2].setItemDamage(var4);
						}
					} else if(var3 != var4) {
						this.brewingItemStacks[var2].setItemDamage(var4);
					}
				}
			}

			if(Item.itemsList[var1.itemID].hasContainerItem()) {
				this.brewingItemStacks[3] = new ItemStack(Item.itemsList[var1.itemID].getContainerItem());
			} else {
				--this.brewingItemStacks[3].stackSize;
				if(this.brewingItemStacks[3].stackSize <= 0) {
					this.brewingItemStacks[3] = null;
				}
			}

		}
	}

	private int getPotionResult(int var1, ItemStack var2) {
		return var2 == null ? var1 : (Item.itemsList[var2.itemID].isPotionIngredient() ? PotionHelper.applyIngredient(var1, Item.itemsList[var2.itemID].getPotionEffect()) : var1);
	}

	public void readFromNBT(NBTTagCompound var1) {
		super.readFromNBT(var1);
		NBTTagList var2 = var1.getTagList("Items");
		this.brewingItemStacks = new ItemStack[this.getSizeInventory()];

		for(int var3 = 0; var3 < var2.tagCount(); ++var3) {
			NBTTagCompound var4 = (NBTTagCompound)var2.tagAt(var3);
			byte var5 = var4.getByte("Slot");
			if(var5 >= 0 && var5 < this.brewingItemStacks.length) {
				this.brewingItemStacks[var5] = ItemStack.loadItemStackFromNBT(var4);
			}
		}

		this.brewTime = var1.getShort("BrewTime");
	}

	public void writeToNBT(NBTTagCompound var1) {
		super.writeToNBT(var1);
		var1.setShort("BrewTime", (short)this.brewTime);
		NBTTagList var2 = new NBTTagList();

		for(int var3 = 0; var3 < this.brewingItemStacks.length; ++var3) {
			if(this.brewingItemStacks[var3] != null) {
				NBTTagCompound var4 = new NBTTagCompound();
				var4.setByte("Slot", (byte)var3);
				this.brewingItemStacks[var3].writeToNBT(var4);
				var2.appendTag(var4);
			}
		}

		var1.setTag("Items", var2);
	}

	public ItemStack getStackInSlot(int var1) {
		return var1 >= 0 && var1 < this.brewingItemStacks.length ? this.brewingItemStacks[var1] : null;
	}

	public ItemStack decrStackSize(int var1, int var2) {
		if(var1 >= 0 && var1 < this.brewingItemStacks.length) {
			ItemStack var3 = this.brewingItemStacks[var1];
			this.brewingItemStacks[var1] = null;
			return var3;
		} else {
			return null;
		}
	}

	public ItemStack getStackInSlotOnClosing(int var1) {
		if(var1 >= 0 && var1 < this.brewingItemStacks.length) {
			ItemStack var2 = this.brewingItemStacks[var1];
			this.brewingItemStacks[var1] = null;
			return var2;
		} else {
			return null;
		}
	}

	public void setInventorySlotContents(int var1, ItemStack var2) {
		if(var1 >= 0 && var1 < this.brewingItemStacks.length) {
			this.brewingItemStacks[var1] = var2;
		}

	}

	public int getInventoryStackLimit() {
		return 1;
	}

	public boolean isUseableByPlayer(EntityPlayer var1) {
		return this.worldObj.getBlockTileEntity(this.xCoord, this.yCoord, this.zCoord) != this ? false : var1.getDistanceSq((double)this.xCoord + 0.5D, (double)this.yCoord + 0.5D, (double)this.zCoord + 0.5D) <= 64.0D;
	}

	public void openChest() {
	}

	public void closeChest() {
	}

	public void setBrewTime(int var1) {
		this.brewTime = var1;
	}

	public int getFilledSlots() {
		int var1 = 0;

		for(int var2 = 0; var2 < 3; ++var2) {
			if(this.brewingItemStacks[var2] != null) {
				var1 |= 1 << var2;
			}
		}

		return var1;
	}
}
