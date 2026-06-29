package net.minecraft.src;

public class EntityMooshroom extends EntityCow {
	public EntityMooshroom(World var1) {
		super(var1);
		this.texture = "/mob/redcow.png";
		this.setSize(0.9F, 1.3F);
	}

	public boolean interact(EntityPlayer var1) {
		ItemStack var2 = var1.inventory.getCurrentItem();
		if(var2 != null && var2.itemID == Item.bowlEmpty.shiftedIndex && this.getGrowingAge() >= 0) {
			if(var2.stackSize == 1) {
				var1.inventory.setInventorySlotContents(var1.inventory.currentItem, new ItemStack(Item.bowlSoup));
				return true;
			}

			if(var1.inventory.addItemStackToInventory(new ItemStack(Item.bowlSoup)) && !var1.capabilities.isCreativeMode) {
				var1.inventory.decrStackSize(var1.inventory.currentItem, 1);
				return true;
			}
		}

		if(var2 != null && var2.itemID == Item.shears.shiftedIndex && this.getGrowingAge() >= 0) {
			this.setDead();
			this.worldObj.spawnParticle("largeexplode", this.posX, this.posY + (double)(this.height / 2.0F), this.posZ, 0.0D, 0.0D, 0.0D);
			if(!this.worldObj.isRemote) {
				EntityCow var3 = new EntityCow(this.worldObj);
				var3.setLocationAndAngles(this.posX, this.posY, this.posZ, this.rotationYaw, this.rotationPitch);
				var3.setEntityHealth(this.getHealth());
				var3.renderYawOffset = this.renderYawOffset;
				this.worldObj.spawnEntityInWorld(var3);

				for(int var4 = 0; var4 < 5; ++var4) {
					this.worldObj.spawnEntityInWorld(new EntityItem(this.worldObj, this.posX, this.posY + (double)this.height, this.posZ, new ItemStack(Block.mushroomRed)));
				}
			}

			return true;
		} else {
			return super.interact(var1);
		}
	}

	public EntityAnimal spawnBabyAnimal(EntityAnimal var1) {
		return new EntityMooshroom(this.worldObj);
	}
}
