package net.minecraft.src;

public class EntitySnowman extends EntityGolem {
	public EntitySnowman(World var1) {
		super(var1);
		this.texture = "/mob/snowman.png";
		this.setSize(0.4F, 1.8F);
		this.getNavigator().func_48664_a(true);
		this.tasks.addTask(1, new EntityAIArrowAttack(this, 0.25F, 2, 20));
		this.tasks.addTask(2, new EntityAIWander(this, 0.2F));
		this.tasks.addTask(3, new EntityAIWatchClosest(this, EntityPlayer.class, 6.0F));
		this.tasks.addTask(4, new EntityAILookIdle(this));
		this.targetTasks.addTask(1, new EntityAINearestAttackableTarget(this, EntityMob.class, 16.0F, 0, true));
	}

	public boolean isAIEnabled() {
		return true;
	}

	public int getMaxHealth() {
		return 4;
	}

	public void onLivingUpdate() {
		super.onLivingUpdate();
		if(this.isWet()) {
			this.attackEntityFrom(DamageSource.drown, 1);
		}

		int var1 = MathHelper.floor_double(this.posX);
		int var2 = MathHelper.floor_double(this.posZ);
		if(this.worldObj.getBiomeGenForCoords(var1, var2).getFloatTemperature() > 1.0F) {
			this.attackEntityFrom(DamageSource.onFire, 1);
		}

		for(var1 = 0; var1 < 4; ++var1) {
			var2 = MathHelper.floor_double(this.posX + (double)((float)(var1 % 2 * 2 - 1) * 0.25F));
			int var3 = MathHelper.floor_double(this.posY);
			int var4 = MathHelper.floor_double(this.posZ + (double)((float)(var1 / 2 % 2 * 2 - 1) * 0.25F));
			if(this.worldObj.getBlockId(var2, var3, var4) == 0 && this.worldObj.getBiomeGenForCoords(var2, var4).getFloatTemperature() < 0.8F && Block.snow.canPlaceBlockAt(this.worldObj, var2, var3, var4)) {
				this.worldObj.setBlockWithNotify(var2, var3, var4, Block.snow.blockID);
			}
		}

	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
	}

	protected int getDropItemId() {
		return Item.snowball.shiftedIndex;
	}

	protected void dropFewItems(boolean var1, int var2) {
		int var3 = this.rand.nextInt(16);

		for(int var4 = 0; var4 < var3; ++var4) {
			this.dropItem(Item.snowball.shiftedIndex, 1);
		}

	}
}
