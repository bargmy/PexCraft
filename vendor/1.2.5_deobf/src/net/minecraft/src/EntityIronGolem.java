package net.minecraft.src;

public class EntityIronGolem extends EntityGolem {
	private int field_48119_b = 0;
	Village villageObj = null;
	private int field_48120_c;
	private int field_48118_d;

	public EntityIronGolem(World var1) {
		super(var1);
		this.texture = "/mob/villager_golem.png";
		this.setSize(1.4F, 2.9F);
		this.getNavigator().func_48664_a(true);
		this.tasks.addTask(1, new EntityAIAttackOnCollide(this, 0.25F, true));
		this.tasks.addTask(2, new EntityAIMoveTowardsTarget(this, 0.22F, 32.0F));
		this.tasks.addTask(3, new EntityAIMoveThroughVillage(this, 0.16F, true));
		this.tasks.addTask(4, new EntityAIMoveTwardsRestriction(this, 0.16F));
		this.tasks.addTask(5, new EntityAILookAtVillager(this));
		this.tasks.addTask(6, new EntityAIWander(this, 0.16F));
		this.tasks.addTask(7, new EntityAIWatchClosest(this, EntityPlayer.class, 6.0F));
		this.tasks.addTask(8, new EntityAILookIdle(this));
		this.targetTasks.addTask(1, new EntityAIDefendVillage(this));
		this.targetTasks.addTask(2, new EntityAIHurtByTarget(this, false));
		this.targetTasks.addTask(3, new EntityAINearestAttackableTarget(this, EntityMob.class, 16.0F, 0, false, true));
	}

	protected void entityInit() {
		super.entityInit();
		this.dataWatcher.addObject(16, Byte.valueOf((byte)0));
	}

	public boolean isAIEnabled() {
		return true;
	}

	protected void updateAITick() {
		if(--this.field_48119_b <= 0) {
			this.field_48119_b = 70 + this.rand.nextInt(50);
			this.villageObj = this.worldObj.villageCollectionObj.findNearestVillage(MathHelper.floor_double(this.posX), MathHelper.floor_double(this.posY), MathHelper.floor_double(this.posZ), 32);
			if(this.villageObj == null) {
				this.detachHome();
			} else {
				ChunkCoordinates var1 = this.villageObj.getCenter();
				this.setHomeArea(var1.posX, var1.posY, var1.posZ, this.villageObj.getVillageRadius());
			}
		}

		super.updateAITick();
	}

	public int getMaxHealth() {
		return 100;
	}

	protected int decreaseAirSupply(int var1) {
		return var1;
	}

	public void onLivingUpdate() {
		super.onLivingUpdate();
		if(this.field_48120_c > 0) {
			--this.field_48120_c;
		}

		if(this.field_48118_d > 0) {
			--this.field_48118_d;
		}

		if(this.motionX * this.motionX + this.motionZ * this.motionZ > (double)2.5000003E-7F && this.rand.nextInt(5) == 0) {
			int var1 = MathHelper.floor_double(this.posX);
			int var2 = MathHelper.floor_double(this.posY - (double)0.2F - (double)this.yOffset);
			int var3 = MathHelper.floor_double(this.posZ);
			int var4 = this.worldObj.getBlockId(var1, var2, var3);
			if(var4 > 0) {
				this.worldObj.spawnParticle("tilecrack_" + var4, this.posX + ((double)this.rand.nextFloat() - 0.5D) * (double)this.width, this.boundingBox.minY + 0.1D, this.posZ + ((double)this.rand.nextFloat() - 0.5D) * (double)this.width, 4.0D * ((double)this.rand.nextFloat() - 0.5D), 0.5D, ((double)this.rand.nextFloat() - 0.5D) * 4.0D);
			}
		}

	}

	public boolean func_48100_a(Class var1) {
		return this.func_48112_E_() && EntityPlayer.class.isAssignableFrom(var1) ? false : super.func_48100_a(var1);
	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
		var1.setBoolean("PlayerCreated", this.func_48112_E_());
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
		this.func_48115_b(var1.getBoolean("PlayerCreated"));
	}

	public boolean attackEntityAsMob(Entity var1) {
		this.field_48120_c = 10;
		this.worldObj.setEntityState(this, (byte)4);
		boolean var2 = var1.attackEntityFrom(DamageSource.causeMobDamage(this), 7 + this.rand.nextInt(15));
		if(var2) {
			var1.motionY += (double)0.4F;
		}

		this.worldObj.playSoundAtEntity(this, "mob.irongolem.throw", 1.0F, 1.0F);
		return var2;
	}

	public void handleHealthUpdate(byte var1) {
		if(var1 == 4) {
			this.field_48120_c = 10;
			this.worldObj.playSoundAtEntity(this, "mob.irongolem.throw", 1.0F, 1.0F);
		} else if(var1 == 11) {
			this.field_48118_d = 400;
		} else {
			super.handleHealthUpdate(var1);
		}

	}

	public Village getVillage() {
		return this.villageObj;
	}

	public int func_48114_ab() {
		return this.field_48120_c;
	}

	public void func_48116_a(boolean var1) {
		this.field_48118_d = var1 ? 400 : 0;
		this.worldObj.setEntityState(this, (byte)11);
	}

	protected String getLivingSound() {
		return "none";
	}

	protected String getHurtSound() {
		return "mob.irongolem.hit";
	}

	protected String getDeathSound() {
		return "mob.irongolem.death";
	}

	protected void playStepSound(int var1, int var2, int var3, int var4) {
		this.worldObj.playSoundAtEntity(this, "mob.irongolem.walk", 1.0F, 1.0F);
	}

	protected void dropFewItems(boolean var1, int var2) {
		int var3 = this.rand.nextInt(3);

		int var4;
		for(var4 = 0; var4 < var3; ++var4) {
			this.dropItem(Block.plantRed.blockID, 1);
		}

		var4 = 3 + this.rand.nextInt(3);

		for(int var5 = 0; var5 < var4; ++var5) {
			this.dropItem(Item.ingotIron.shiftedIndex, 1);
		}

	}

	public int func_48117_D_() {
		return this.field_48118_d;
	}

	public boolean func_48112_E_() {
		return (this.dataWatcher.getWatchableObjectByte(16) & 1) != 0;
	}

	public void func_48115_b(boolean var1) {
		byte var2 = this.dataWatcher.getWatchableObjectByte(16);
		if(var1) {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 | 1)));
		} else {
			this.dataWatcher.updateObject(16, Byte.valueOf((byte)(var2 & -2)));
		}

	}
}
