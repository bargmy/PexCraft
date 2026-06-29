package net.minecraft.src;

public class InventoryPlayer implements IInventory {
	public ItemStack[] mainInventory = new ItemStack[36];
	public ItemStack[] armorInventory = new ItemStack[4];
	public int currentItem = 0;
	public EntityPlayer player;
	private ItemStack itemStack;
	public boolean inventoryChanged = false;

	public InventoryPlayer(EntityPlayer var1) {
		this.player = var1;
	}

	public ItemStack getCurrentItem() {
		return this.currentItem < 9 && this.currentItem >= 0 ? this.mainInventory[this.currentItem] : null;
	}

	private int getInventorySlotContainItem(int var1) {
		for(int var2 = 0; var2 < this.mainInventory.length; ++var2) {
			if(this.mainInventory[var2] != null && this.mainInventory[var2].itemID == var1) {
				return var2;
			}
		}

		return -1;
	}

	private int getInventorySlotContainItemAndDamage(int var1, int var2) {
		for(int var3 = 0; var3 < this.mainInventory.length; ++var3) {
			if(this.mainInventory[var3] != null && this.mainInventory[var3].itemID == var1 && this.mainInventory[var3].getItemDamage() == var2) {
				return var3;
			}
		}

		return -1;
	}

	private int storeItemStack(ItemStack var1) {
		for(int var2 = 0; var2 < this.mainInventory.length; ++var2) {
			if(this.mainInventory[var2] != null && this.mainInventory[var2].itemID == var1.itemID && this.mainInventory[var2].isStackable() && this.mainInventory[var2].stackSize < this.mainInventory[var2].getMaxStackSize() && this.mainInventory[var2].stackSize < this.getInventoryStackLimit() && (!this.mainInventory[var2].getHasSubtypes() || this.mainInventory[var2].getItemDamage() == var1.getItemDamage()) && ItemStack.func_46154_a(this.mainInventory[var2], var1)) {
				return var2;
			}
		}

		return -1;
	}

	private int getFirstEmptyStack() {
		for(int var1 = 0; var1 < this.mainInventory.length; ++var1) {
			if(this.mainInventory[var1] == null) {
				return var1;
			}
		}

		return -1;
	}

	public void setCurrentItem(int var1, int var2, boolean var3, boolean var4) {
		boolean var5 = true;
		int var7;
		if(var3) {
			var7 = this.getInventorySlotContainItemAndDamage(var1, var2);
		} else {
			var7 = this.getInventorySlotContainItem(var1);
		}

		if(var7 >= 0 && var7 < 9) {
			this.currentItem = var7;
		} else {
			if(var4 && var1 > 0) {
				int var6 = this.getFirstEmptyStack();
				if(var6 >= 0 && var6 < 9) {
					this.currentItem = var6;
				}

				this.func_52006_a(Item.itemsList[var1], var2);
			}

		}
	}

	public void changeCurrentItem(int var1) {
		if(var1 > 0) {
			var1 = 1;
		}

		if(var1 < 0) {
			var1 = -1;
		}

		for(this.currentItem -= var1; this.currentItem < 0; this.currentItem += 9) {
		}

		while(this.currentItem >= 9) {
			this.currentItem -= 9;
		}

	}

	public void func_52006_a(Item var1, int var2) {
		if(var1 != null) {
			int var3 = this.getInventorySlotContainItemAndDamage(var1.shiftedIndex, var2);
			if(var3 >= 0) {
				this.mainInventory[var3] = this.mainInventory[this.currentItem];
			}

			this.mainInventory[this.currentItem] = new ItemStack(Item.itemsList[var1.shiftedIndex], 1, var2);
		}

	}

	private int storePartialItemStack(ItemStack var1) {
		int var2 = var1.itemID;
		int var3 = var1.stackSize;
		int var4;
		if(var1.getMaxStackSize() == 1) {
			var4 = this.getFirstEmptyStack();
			if(var4 < 0) {
				return var3;
			} else {
				if(this.mainInventory[var4] == null) {
					this.mainInventory[var4] = ItemStack.copyItemStack(var1);
				}

				return 0;
			}
		} else {
			var4 = this.storeItemStack(var1);
			if(var4 < 0) {
				var4 = this.getFirstEmptyStack();
			}

			if(var4 < 0) {
				return var3;
			} else {
				if(this.mainInventory[var4] == null) {
					this.mainInventory[var4] = new ItemStack(var2, 0, var1.getItemDamage());
					if(var1.hasTagCompound()) {
						this.mainInventory[var4].setTagCompound((NBTTagCompound)var1.getTagCompound().copy());
					}
				}

				int var5 = var3;
				if(var3 > this.mainInventory[var4].getMaxStackSize() - this.mainInventory[var4].stackSize) {
					var5 = this.mainInventory[var4].getMaxStackSize() - this.mainInventory[var4].stackSize;
				}

				if(var5 > this.getInventoryStackLimit() - this.mainInventory[var4].stackSize) {
					var5 = this.getInventoryStackLimit() - this.mainInventory[var4].stackSize;
				}

				if(var5 == 0) {
					return var3;
				} else {
					var3 -= var5;
					this.mainInventory[var4].stackSize += var5;
					this.mainInventory[var4].animationsToGo = 5;
					return var3;
				}
			}
		}
	}

	public void decrementAnimations() {
		for(int var1 = 0; var1 < this.mainInventory.length; ++var1) {
			if(this.mainInventory[var1] != null) {
				this.mainInventory[var1].updateAnimation(this.player.worldObj, this.player, var1, this.currentItem == var1);
			}
		}

	}

	public boolean consumeInventoryItem(int var1) {
		int var2 = this.getInventorySlotContainItem(var1);
		if(var2 < 0) {
			return false;
		} else {
			if(--this.mainInventory[var2].stackSize <= 0) {
				this.mainInventory[var2] = null;
			}

			return true;
		}
	}

	public boolean hasItem(int var1) {
		int var2 = this.getInventorySlotContainItem(var1);
		return var2 >= 0;
	}

	public boolean addItemStackToInventory(ItemStack var1) {
		int var2;
		if(var1.isItemDamaged()) {
			var2 = this.getFirstEmptyStack();
			if(var2 >= 0) {
				this.mainInventory[var2] = ItemStack.copyItemStack(var1);
				this.mainInventory[var2].animationsToGo = 5;
				var1.stackSize = 0;
				return true;
			} else if(this.player.capabilities.isCreativeMode) {
				var1.stackSize = 0;
				return true;
			} else {
				return false;
			}
		} else {
			do {
				var2 = var1.stackSize;
				var1.stackSize = this.storePartialItemStack(var1);
			} while(var1.stackSize > 0 && var1.stackSize < var2);

			if(var1.stackSize == var2 && this.player.capabilities.isCreativeMode) {
				var1.stackSize = 0;
				return true;
			} else {
				return var1.stackSize < var2;
			}
		}
	}

	public ItemStack decrStackSize(int var1, int var2) {
		ItemStack[] var3 = this.mainInventory;
		if(var1 >= this.mainInventory.length) {
			var3 = this.armorInventory;
			var1 -= this.mainInventory.length;
		}

		if(var3[var1] != null) {
			ItemStack var4;
			if(var3[var1].stackSize <= var2) {
				var4 = var3[var1];
				var3[var1] = null;
				return var4;
			} else {
				var4 = var3[var1].splitStack(var2);
				if(var3[var1].stackSize == 0) {
					var3[var1] = null;
				}

				return var4;
			}
		} else {
			return null;
		}
	}

	public ItemStack getStackInSlotOnClosing(int var1) {
		ItemStack[] var2 = this.mainInventory;
		if(var1 >= this.mainInventory.length) {
			var2 = this.armorInventory;
			var1 -= this.mainInventory.length;
		}

		if(var2[var1] != null) {
			ItemStack var3 = var2[var1];
			var2[var1] = null;
			return var3;
		} else {
			return null;
		}
	}

	public void setInventorySlotContents(int var1, ItemStack var2) {
		ItemStack[] var3 = this.mainInventory;
		if(var1 >= var3.length) {
			var1 -= var3.length;
			var3 = this.armorInventory;
		}

		var3[var1] = var2;
	}

	public float getStrVsBlock(Block var1) {
		float var2 = 1.0F;
		if(this.mainInventory[this.currentItem] != null) {
			var2 *= this.mainInventory[this.currentItem].getStrVsBlock(var1);
		}

		return var2;
	}

	public NBTTagList writeToNBT(NBTTagList var1) {
		int var2;
		NBTTagCompound var3;
		for(var2 = 0; var2 < this.mainInventory.length; ++var2) {
			if(this.mainInventory[var2] != null) {
				var3 = new NBTTagCompound();
				var3.setByte("Slot", (byte)var2);
				this.mainInventory[var2].writeToNBT(var3);
				var1.appendTag(var3);
			}
		}

		for(var2 = 0; var2 < this.armorInventory.length; ++var2) {
			if(this.armorInventory[var2] != null) {
				var3 = new NBTTagCompound();
				var3.setByte("Slot", (byte)(var2 + 100));
				this.armorInventory[var2].writeToNBT(var3);
				var1.appendTag(var3);
			}
		}

		return var1;
	}

	public void readFromNBT(NBTTagList var1) {
		this.mainInventory = new ItemStack[36];
		this.armorInventory = new ItemStack[4];

		for(int var2 = 0; var2 < var1.tagCount(); ++var2) {
			NBTTagCompound var3 = (NBTTagCompound)var1.tagAt(var2);
			int var4 = var3.getByte("Slot") & 255;
			ItemStack var5 = ItemStack.loadItemStackFromNBT(var3);
			if(var5 != null) {
				if(var4 >= 0 && var4 < this.mainInventory.length) {
					this.mainInventory[var4] = var5;
				}

				if(var4 >= 100 && var4 < this.armorInventory.length + 100) {
					this.armorInventory[var4 - 100] = var5;
				}
			}
		}

	}

	public int getSizeInventory() {
		return this.mainInventory.length + 4;
	}

	public ItemStack getStackInSlot(int var1) {
		ItemStack[] var2 = this.mainInventory;
		if(var1 >= var2.length) {
			var1 -= var2.length;
			var2 = this.armorInventory;
		}

		return var2[var1];
	}

	public String getInvName() {
		return "container.inventory";
	}

	public int getInventoryStackLimit() {
		return 64;
	}

	public int getDamageVsEntity(Entity var1) {
		ItemStack var2 = this.getStackInSlot(this.currentItem);
		return var2 != null ? var2.getDamageVsEntity(var1) : 1;
	}

	public boolean canHarvestBlock(Block var1) {
		if(var1.blockMaterial.isHarvestable()) {
			return true;
		} else {
			ItemStack var2 = this.getStackInSlot(this.currentItem);
			return var2 != null ? var2.canHarvestBlock(var1) : false;
		}
	}

	public ItemStack armorItemInSlot(int var1) {
		return this.armorInventory[var1];
	}

	public int getTotalArmorValue() {
		int var1 = 0;

		for(int var2 = 0; var2 < this.armorInventory.length; ++var2) {
			if(this.armorInventory[var2] != null && this.armorInventory[var2].getItem() instanceof ItemArmor) {
				int var3 = ((ItemArmor)this.armorInventory[var2].getItem()).damageReduceAmount;
				var1 += var3;
			}
		}

		return var1;
	}

	public void damageArmor(int var1) {
		var1 /= 4;
		if(var1 < 1) {
			var1 = 1;
		}

		for(int var2 = 0; var2 < this.armorInventory.length; ++var2) {
			if(this.armorInventory[var2] != null && this.armorInventory[var2].getItem() instanceof ItemArmor) {
				this.armorInventory[var2].damageItem(var1, this.player);
				if(this.armorInventory[var2].stackSize == 0) {
					this.armorInventory[var2].onItemDestroyedByUse(this.player);
					this.armorInventory[var2] = null;
				}
			}
		}

	}

	public void dropAllItems() {
		int var1;
		for(var1 = 0; var1 < this.mainInventory.length; ++var1) {
			if(this.mainInventory[var1] != null) {
				this.player.dropPlayerItemWithRandomChoice(this.mainInventory[var1], true);
				this.mainInventory[var1] = null;
			}
		}

		for(var1 = 0; var1 < this.armorInventory.length; ++var1) {
			if(this.armorInventory[var1] != null) {
				this.player.dropPlayerItemWithRandomChoice(this.armorInventory[var1], true);
				this.armorInventory[var1] = null;
			}
		}

	}

	public void onInventoryChanged() {
		this.inventoryChanged = true;
	}

	public void setItemStack(ItemStack var1) {
		this.itemStack = var1;
		this.player.onItemStackChanged(var1);
	}

	public ItemStack getItemStack() {
		return this.itemStack;
	}

	public boolean isUseableByPlayer(EntityPlayer var1) {
		return this.player.isDead ? false : var1.getDistanceSqToEntity(this.player) <= 64.0D;
	}

	public boolean hasItemStack(ItemStack var1) {
		int var2;
		for(var2 = 0; var2 < this.armorInventory.length; ++var2) {
			if(this.armorInventory[var2] != null && this.armorInventory[var2].isStackEqual(var1)) {
				return true;
			}
		}

		for(var2 = 0; var2 < this.mainInventory.length; ++var2) {
			if(this.mainInventory[var2] != null && this.mainInventory[var2].isStackEqual(var1)) {
				return true;
			}
		}

		return false;
	}

	public void openChest() {
	}

	public void closeChest() {
	}

	public void copyInventory(InventoryPlayer var1) {
		int var2;
		for(var2 = 0; var2 < this.mainInventory.length; ++var2) {
			this.mainInventory[var2] = ItemStack.copyItemStack(var1.mainInventory[var2]);
		}

		for(var2 = 0; var2 < this.armorInventory.length; ++var2) {
			this.armorInventory[var2] = ItemStack.copyItemStack(var1.armorInventory[var2]);
		}

	}
}
