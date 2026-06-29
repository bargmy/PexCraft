package net.minecraft.src;

public abstract class EntityTameable extends EntityAnimal {
	protected EntityAISit aiSit = new EntityAISit(this);

	public EntityTameable(World var1) {
		super(var1);
	}

	protected void entityInit() {
		super.entityInit();
		this.dataWatcher.addObject(16, Byte.valueOf((byte)0));
		this.dataWatcher.addObject(17, "");
	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
		if(this.getOwnerName() == null) {
			var1.setString("Owner", "");
		} else {
			var1.setString("Owner", this.getOwnerName());
		}

		var1.setBoolean("Sitting", this.isSitting());
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
		String var2 = var1.getString("Owner");
		if(var2.length() > 0) {
			this.setOwner(var2);
			this.setTamed(true);
		}

		this.aiSit.func_48407_a(var1.getBoolean("Sitting"));
	}

	protected void func_48142_a(boolean var1) {
		String var2 = "heart";
		if(!var1) {
			var2 = "smoke";
		}

		for(int var3 = 0; var3 < 7; ++var3) {
			double var4 = this.rand.nextGaussian() * 0.02D;
			double var6 = this.rand.nextGaussian() * 0.02D;
			double var8 = this.rand.nextGaussian() * 0.02D;
			this.worldObj.spawnParticle(var2, this.posX + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, this.posY + 0.5D + (double)(this.rand.nextFloat() * this.height), this.posZ + (double)(this.rand.nextFloat() * this.width * 2.0F) - (double)this.width, var4, var6, var8);
		}

	}

	public void handleHealthUpdate(byte var1) {
		if(var1 == 7) {
			this.func_48142_a(true);
		} else if(var1 == 6) {
			this.func_48142_a(false);
		} else {
			super.handleHealthUpdate(var1);
		}

	}

	public boolean isTamed() {
		return (this.dataWatcher.getWatchableObjectByte(16) & 4) != 0;
	}

	public void setTamed(boolean var1) {
		byte var2 = this.dataWatcher.getWatchableObjectByte(16);
		if(var1) {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 | 4)));
		} else {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 & -5)));
		}

	}

	public boolean isSitting() {
		return (this.dataWatcher.getWatchableObjectByte(16) & 1) != 0;
	}

	public void func_48140_f(boolean var1) {
		byte var2 = this.dataWatcher.getWatchableObjectByte(16);
		if(var1) {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 | 1)));
		} else {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 & -2)));
		}

	}

	public String getOwnerName() {
		return this.dataWatcher.getWatchableObjectString(17);
	}

	public void setOwner(String var1) {
		this.dataWatcher.updateObject(17, var1);
	}

	public EntityLiving getOwner() {
		return this.worldObj.getPlayerEntityByName(this.getOwnerName());
	}

	public EntityAISit func_50008_ai() {
		return this.aiSit;
	}
}
