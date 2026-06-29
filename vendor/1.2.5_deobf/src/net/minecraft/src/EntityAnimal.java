package net.minecraft.src;

import java.util.List;

public abstract class EntityAnimal extends EntityAgeable {
	private int inLove;
	private int breeding = 0;

	public EntityAnimal(World var1) {
		super(var1);
	}

	protected void updateAITick() {
		if(this.getGrowingAge() != 0) {
			this.inLove = 0;
		}

		super.updateAITick();
	}

	public void onLivingUpdate() {
		super.onLivingUpdate();
		if(this.getGrowingAge() != 0) {
			this.inLove = 0;
		}

		if(this.inLove > 0) {
			--this.inLove;
			String var1 = "heart";
			if(this.inLove % 10 == 0) {
				double var2 = this.rand.nextGaussian() * 0.02D;
				double var4 = this.rand.nextGaussian() * 0.02D;
				double var6 = this.rand.nextGaussian() * 0.02D;
				this.worldObj.spawnParticle(var1, this.posX + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, this.posY + 0.5D + (double)(this.rand.nextFloat() * this.height), this.posZ + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, var2, var4, var6);
			}
		} else {
			this.breeding = 0;
		}

	}

	protected void attackEntity(Entity var1, float var2) {
		if(var1 instanceof EntityPlayer) {
			if(var2 < 3.0F) {
				double var3 = var1.posX - this.posX;
				double var5 = var1.posZ - this.posZ;
				this.rotationYaw = (float)(Math.atan2(var5, var3) * 180.0D / (double)((float)Math.PI)) - 90.0F;
				this.hasAttacked = true;
			}

			EntityPlayer var7 = (EntityPlayer)var1;
			if(var7.getCurrentEquippedItem() == null || !this.isWheat(var7.getCurrentEquippedItem())) {
				this.entityToAttack = null;
			}
		} else if(var1 instanceof EntityAnimal) {
			EntityAnimal var8 = (EntityAnimal)var1;
			if(this.getGrowingAge() > 0 && var8.getGrowingAge() < 0) {
				if((double)var2 < 2.5D) {
					this.hasAttacked = true;
				}
			} else if(this.inLove > 0 && var8.inLove > 0) {
				if(var8.entityToAttack == null) {
					var8.entityToAttack = this;
				}

				if(var8.entityToAttack == this && (double)var2 < 3.5D) {
					++var8.inLove;
					++this.inLove;
					++this.breeding;
					if(this.breeding % 4 == 0) {
						this.worldObj.spawnParticle("heart", this.posX + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, this.posY + 0.5D + (double)(this.rand.nextFloat() * this.height), this.posZ + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, 0.0D, 0.0D, 0.0D);
					}

					if(this.breeding == 60) {
						this.procreate((EntityAnimal)var1);
					}
				} else {
					this.breeding = 0;
				}
			} else {
				this.breeding = 0;
				this.entityToAttack = null;
			}
		}

	}

	private void procreate(EntityAnimal var1) {
		EntityAnimal var2 = this.spawnBabyAnimal(var1);
		if(var2 != null) {
			this.setGrowingAge(6000);
			var1.setGrowingAge(6000);
			this.inLove = 0;
			this.breeding = 0;
			this.entityToAttack = null;
			var1.entityToAttack = null;
			var1.breeding = 0;
			var1.inLove = 0;
			var2.setGrowingAge(-24000);
			var2.setLocationAndAngles(this.posX, this.posY, this.posZ, this.rotationYaw, this.rotationPitch);

			for(int var3 = 0; var3 < 7; ++var3) {
				double var4 = this.rand.nextGaussian() * 0.02D;
				double var6 = this.rand.nextGaussian() * 0.02D;
				double var8 = this.rand.nextGaussian() * 0.02D;
				this.worldObj.spawnParticle("heart", this.posX + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, this.posY + 0.5D + (double)(this.rand.nextFloat() * this.height), this.posZ + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, var4, var6, var8);
			}

			this.worldObj.spawnEntityInWorld(var2);
		}

	}

	public abstract EntityAnimal spawnBabyAnimal(EntityAnimal var1);

	protected void attackBlockedEntity(Entity var1, float var2) {
	}

	public boolean attackEntityFrom(DamageSource var1, int var2) {
		this.fleeingTick = 60;
		this.entityToAttack = null;
		this.inLove = 0;
		return super.attackEntityFrom(var1, var2);
	}

	public float getBlockPathWeight(int var1, int var2, int var3) {
		return this.worldObj.getBlockId(var1, var2 - 1, var3) == Block.grass.blockID ? 10.0F : this.worldObj.getLightBrightness(var1, var2, var3) - 0.5F;
	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
		var1.setInteger("InLove", this.inLove);
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
		this.inLove = var1.getInteger("InLove");
	}

	protected Entity findPlayerToAttack() {
		if(this.fleeingTick > 0) {
			return null;
		} else {
			float var1 = 8.0F;
			List var2;
			int var3;
			EntityAnimal var4;
			if(this.inLove > 0) {
				var2 = this.worldObj.getEntitiesWithinAABB(this.getClass(), this.boundingBox.expand((double)var1, (double)var1, (double)var1));

				for(var3 = 0; var3 < var2.size(); ++var3) {
					var4 = (EntityAnimal)var2.get(var3);
					if(var4 != this && var4.inLove > 0) {
						return var4;
					}
				}
			} else if(this.getGrowingAge() == 0) {
				var2 = this.worldObj.getEntitiesWithinAABB(EntityPlayer.class, this.boundingBox.expand((double)var1, (double)var1, (double)var1));

				for(var3 = 0; var3 < var2.size(); ++var3) {
					EntityPlayer var5 = (EntityPlayer)var2.get(var3);
					if(var5.getCurrentEquippedItem() != null && this.isWheat(var5.getCurrentEquippedItem())) {
						return var5;
					}
				}
			} else if(this.getGrowingAge() > 0) {
				var2 = this.worldObj.getEntitiesWithinAABB(this.getClass(), this.boundingBox.expand((double)var1, (double)var1, (double)var1));

				for(var3 = 0; var3 < var2.size(); ++var3) {
					var4 = (EntityAnimal)var2.get(var3);
					if(var4 != this && var4.getGrowingAge() < 0) {
						return var4;
					}
				}
			}

			return null;
		}
	}

	public boolean getCanSpawnHere() {
		int var1 = MathHelper.floor_double(this.posX);
		int var2 = MathHelper.floor_double(this.boundingBox.minY);
		int var3 = MathHelper.floor_double(this.posZ);
		return this.worldObj.getBlockId(var1, var2 - 1, var3) == Block.grass.blockID && this.worldObj.getFullBlockLightValue(var1, var2, var3) > 8 && super.getCanSpawnHere();
	}

	public int getTalkInterval() {
		return 120;
	}

	protected boolean canDespawn() {
		return false;
	}

	protected int getExperiencePoints(EntityPlayer var1) {
		return 1 + this.worldObj.rand.nextInt(3);
	}

	public boolean isWheat(ItemStack var1) {
		return var1.itemID == Item.wheat.shiftedIndex;
	}

	public boolean interact(EntityPlayer var1) {
		ItemStack var2 = var1.inventory.getCurrentItem();
		if(var2 != null && this.isWheat(var2) && this.getGrowingAge() == 0) {
			if(!var1.capabilities.isCreativeMode) {
				--var2.stackSize;
				if(var2.stackSize <= 0) {
					var1.inventory.setInventorySlotContents(var1.inventory.currentItem, (ItemStack)null);
				}
			}

			this.inLove = 600;
			this.entityToAttack = null;

			for(int var3 = 0; var3 < 7; ++var3) {
				double var4 = this.rand.nextGaussian() * 0.02D;
				double var6 = this.rand.nextGaussian() * 0.02D;
				double var8 = this.rand.nextGaussian() * 0.02D;
				this.worldObj.spawnParticle("heart", this.posX + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, this.posY + 0.5D + (double)(this.rand.nextFloat() * this.height), this.posZ + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, var4, var6, var8);
			}

			return true;
		} else {
			return super.interact(var1);
		}
	}

	public boolean isInLove() {
		return this.inLove > 0;
	}

	public void resetInLove() {
		this.inLove = 0;
	}

	public boolean func_48135_b(EntityAnimal var1) {
		return var1 == this ? false : (var1.getClass() != this.getClass() ? false : this.isInLove() && var1.isInLove());
	}
}
