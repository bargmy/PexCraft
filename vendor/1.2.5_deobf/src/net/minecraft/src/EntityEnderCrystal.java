package net.minecraft.src;

public class EntityEnderCrystal extends Entity {
	public int innerRotation;
	public int health;

	public EntityEnderCrystal(World var1) {
		super(var1);
		this.innerRotation = 0;
		this.preventEntitySpawning = true;
		this.setSize(2.0F, 2.0F);
		this.yOffset = this.height / 2.0F;
		this.health = 5;
		this.innerRotation = this.rand.nextInt(100000);
	}

	public EntityEnderCrystal(World var1, double var2, double var4, double var6) {
		this(var1);
		this.setPosition(var2, var4, var6);
	}

	protected boolean canTriggerWalking() {
		return false;
	}

	protected void entityInit() {
		this.dataWatcher.addObject(8, Integer.valueOf(this.health));
	}

	public void onUpdate() {
		this.prevPosX = this.posX;
		this.prevPosY = this.posY;
		this.prevPosZ = this.posZ;
		++this.innerRotation;
		this.dataWatcher.updateObject(8, Integer.valueOf(this.health));
		int var1 = MathHelper.floor_double(this.posX);
		int var2 = MathHelper.floor_double(this.posY);
		int var3 = MathHelper.floor_double(this.posZ);
		if(this.worldObj.getBlockId(var1, var2, var3) != Block.fire.blockID) {
			this.worldObj.setBlockWithNotify(var1, var2, var3, Block.fire.blockID);
		}

	}

	protected void writeEntityToNBT(NBTTagCompound var1) {
	}

	protected void readEntityFromNBT(NBTTagCompound var1) {
	}

	public float getShadowSize() {
		return 0.0F;
	}

	public boolean canBeCollidedWith() {
		return true;
	}

	public boolean attackEntityFrom(DamageSource var1, int var2) {
		if(!this.isDead && !this.worldObj.isRemote) {
			this.health = 0;
			if(this.health <= 0) {
				if(!this.worldObj.isRemote) {
					this.setDead();
					this.worldObj.createExplosion((Entity)null, this.posX, this.posY, this.posZ, 6.0F);
				} else {
					this.setDead();
				}
			}
		}

		return true;
	}
}
