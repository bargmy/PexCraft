package net.minecraft.src;

public class TileEntityChest extends TileEntity implements IInventory {
	private ItemStack[] chestContents = new ItemStack[36];
	public boolean adjacentChestChecked = false;
	public TileEntityChest adjacentChestZNeg;
	public TileEntityChest adjacentChestXPos;
	public TileEntityChest adjacentChestXNeg;
	public TileEntityChest adjacentChestZPos;
	public float lidAngle;
	public float prevLidAngle;
	public int numUsingPlayers;
	private int ticksSinceSync;

	public int getSizeInventory() {
		return 27;
	}

	public ItemStack getStackInSlot(int var1) {
		return this.chestContents[var1];
	}

	public ItemStack decrStackSize(int var1, int var2) {
		if(this.chestContents[var1] != null) {
			ItemStack var3;
			if(this.chestContents[var1].stackSize <= var2) {
				var3 = this.chestContents[var1];
				this.chestContents[var1] = null;
				this.onInventoryChanged();
				return var3;
			} else {
				var3 = this.chestContents[var1].splitStack(var2);
				if(this.chestContents[var1].stackSize == 0) {
					this.chestContents[var1] = null;
				}

				this.onInventoryChanged();
				return var3;
			}
		} else {
			return null;
		}
	}

	public ItemStack getStackInSlotOnClosing(int var1) {
		if(this.chestContents[var1] != null) {
			ItemStack var2 = this.chestContents[var1];
			this.chestContents[var1] = null;
			return var2;
		} else {
			return null;
		}
	}

	public void setInventorySlotContents(int var1, ItemStack var2) {
		this.chestContents[var1] = var2;
		if(var2 != null && var2.stackSize > this.getInventoryStackLimit()) {
			var2.stackSize = this.getInventoryStackLimit();
		}

		this.onInventoryChanged();
	}

	public String getInvName() {
		return "container.chest";
	}

	public void readFromNBT(NBTTagCompound var1) {
		super.readFromNBT(var1);
		NBTTagList var2 = var1.getTagList("Items");
		this.chestContents = new ItemStack[this.getSizeInventory()];

		for(int var3 = 0; var3 < var2.tagCount(); ++var3) {
			NBTTagCompound var4 = (NBTTagCompound)var2.tagAt(var3);
			int var5 = var4.getByte("Slot") & 255;
			if(var5 >= 0 && var5 < this.chestContents.length) {
				this.chestContents[var5] = ItemStack.loadItemStackFromNBT(var4);
			}
		}

	}

	public void writeToNBT(NBTTagCompound var1) {
		super.writeToNBT(var1);
		NBTTagList var2 = new NBTTagList();

		for(int var3 = 0; var3 < this.chestContents.length; ++var3) {
			if(this.chestContents[var3] != null) {
				NBTTagCompound var4 = new NBTTagCompound();
				var4.setByte("Slot", (byte)var3);
				this.chestContents[var3].writeToNBT(var4);
				var2.appendTag(var4);
			}
		}

		var1.setTag("Items", var2);
	}

	public int getInventoryStackLimit() {
		return 64;
	}

	public boolean isUseableByPlayer(EntityPlayer var1) {
		return this.worldObj.getBlockTileEntity(this.xCoord, this.yCoord, this.zCoord) != this ? false : var1.getDistanceSq((double)this.xCoord + 0.5D, (double)this.yCoord + 0.5D, (double)this.zCoord + 0.5D) <= 64.0D;
	}

	public void updateContainingBlockInfo() {
		super.updateContainingBlockInfo();
		this.adjacentChestChecked = false;
	}

	public void checkForAdjacentChests() {
		if(!this.adjacentChestChecked) {
			this.adjacentChestChecked = true;
			this.adjacentChestZNeg = null;
			this.adjacentChestXPos = null;
			this.adjacentChestXNeg = null;
			this.adjacentChestZPos = null;
			if(this.worldObj.getBlockId(this.xCoord - 1, this.yCoord, this.zCoord) == Block.chest.blockID) {
				this.adjacentChestXNeg = (TileEntityChest)this.worldObj.getBlockTileEntity(this.xCoord - 1, this.yCoord, this.zCoord);
			}

			if(this.worldObj.getBlockId(this.xCoord + 1, this.yCoord, this.zCoord) == Block.chest.blockID) {
				this.adjacentChestXPos = (TileEntityChest)this.worldObj.getBlockTileEntity(this.xCoord + 1, this.yCoord, this.zCoord);
			}

			if(this.worldObj.getBlockId(this.xCoord, this.yCoord, this.zCoord - 1) == Block.chest.blockID) {
				this.adjacentChestZNeg = (TileEntityChest)this.worldObj.getBlockTileEntity(this.xCoord, this.yCoord, this.zCoord - 1);
			}

			if(this.worldObj.getBlockId(this.xCoord, this.yCoord, this.zCoord + 1) == Block.chest.blockID) {
				this.adjacentChestZPos = (TileEntityChest)this.worldObj.getBlockTileEntity(this.xCoord, this.yCoord, this.zCoord + 1);
			}

			if(this.adjacentChestZNeg != null) {
				this.adjacentChestZNeg.updateContainingBlockInfo();
			}

			if(this.adjacentChestZPos != null) {
				this.adjacentChestZPos.updateContainingBlockInfo();
			}

			if(this.adjacentChestXPos != null) {
				this.adjacentChestXPos.updateContainingBlockInfo();
			}

			if(this.adjacentChestXNeg != null) {
				this.adjacentChestXNeg.updateContainingBlockInfo();
			}

		}
	}

	public void updateEntity() {
		super.updateEntity();
		this.checkForAdjacentChests();
		if(++this.ticksSinceSync % 20 * 4 == 0) {
			this.worldObj.playNoteAt(this.xCoord, this.yCoord, this.zCoord, 1, this.numUsingPlayers);
		}

		this.prevLidAngle = this.lidAngle;
		float var1 = 0.1F;
		double var4;
		if(this.numUsingPlayers > 0 && this.lidAngle == 0.0F && this.adjacentChestZNeg == null && this.adjacentChestXNeg == null) {
			double var2 = (double)this.xCoord + 0.5D;
			var4 = (double)this.zCoord + 0.5D;
			if(this.adjacentChestZPos != null) {
				var4 += 0.5D;
			}

			if(this.adjacentChestXPos != null) {
				var2 += 0.5D;
			}

			this.worldObj.playSoundEffect(var2, (double)this.yCoord + 0.5D, var4, "random.chestopen", 0.5F, this.worldObj.rand.nextFloat() * 0.1F + 0.9F);
		}

		if(this.numUsingPlayers == 0 && this.lidAngle > 0.0F || this.numUsingPlayers > 0 && this.lidAngle < 1.0F) {
			float var8 = this.lidAngle;
			if(this.numUsingPlayers > 0) {
				this.lidAngle += var1;
			} else {
				this.lidAngle -= var1;
			}

			if(this.lidAngle > 1.0F) {
				this.lidAngle = 1.0F;
			}

			float var3 = 0.5F;
			if(this.lidAngle < var3 && var8 >= var3 && this.adjacentChestZNeg == null && this.adjacentChestXNeg == null) {
				var4 = (double)this.xCoord + 0.5D;
				double var6 = (double)this.zCoord + 0.5D;
				if(this.adjacentChestZPos != null) {
					var6 += 0.5D;
				}

				if(this.adjacentChestXPos != null) {
					var4 += 0.5D;
				}

				this.worldObj.playSoundEffect(var4, (double)this.yCoord + 0.5D, var6, "random.chestclosed", 0.5F, this.worldObj.rand.nextFloat() * 0.1F + 0.9F);
			}

			if(this.lidAngle < 0.0F) {
				this.lidAngle = 0.0F;
			}
		}

	}

	public void onTileEntityPowered(int var1, int var2) {
		if(var1 == 1) {
			this.numUsingPlayers = var2;
		}

	}

	public void openChest() {
		++this.numUsingPlayers;
		this.worldObj.playNoteAt(this.xCoord, this.yCoord, this.zCoord, 1, this.numUsingPlayers);
	}

	public void closeChest() {
		--this.numUsingPlayers;
		this.worldObj.playNoteAt(this.xCoord, this.yCoord, this.zCoord, 1, this.numUsingPlayers);
	}

	public void invalidate() {
		this.updateContainingBlockInfo();
		this.checkForAdjacentChests();
		super.invalidate();
	}
}
