package net.minecraft.src;

public class EntityAIArrowAttack extends EntityAIBase {
	World worldObj;
	EntityLiving entityHost;
	EntityLiving attackTarget;
	int rangedAttackTime = 0;
	float field_48370_e;
	int field_48367_f = 0;
	int rangedAttackID;
	int maxRangedAttackTime;

	public EntityAIArrowAttack(EntityLiving var1, float var2, int var3, int var4) {
		this.entityHost = var1;
		this.worldObj = var1.worldObj;
		this.field_48370_e = var2;
		this.rangedAttackID = var3;
		this.maxRangedAttackTime = var4;
		this.setMutexBits(3);
	}

	public boolean shouldExecute() {
		EntityLiving var1 = this.entityHost.getAttackTarget();
		if(var1 == null) {
			return false;
		} else {
			this.attackTarget = var1;
			return true;
		}
	}

	public boolean continueExecuting() {
		return this.shouldExecute() || !this.entityHost.getNavigator().noPath();
	}

	public void resetTask() {
		this.attackTarget = null;
	}

	public void updateTask() {
		double var1 = 100.0D;
		double var3 = this.entityHost.getDistanceSq(this.attackTarget.posX, this.attackTarget.boundingBox.minY, this.attackTarget.posZ);
		boolean var5 = this.entityHost.func_48090_aM().canSee(this.attackTarget);
		if(var5) {
			++this.field_48367_f;
		} else {
			this.field_48367_f = 0;
		}

		if(var3 <= var1 && this.field_48367_f >= 20) {
			this.entityHost.getNavigator().clearPathEntity();
		} else {
			this.entityHost.getNavigator().func_48667_a(this.attackTarget, this.field_48370_e);
		}

		this.entityHost.getLookHelper().setLookPositionWithEntity(this.attackTarget, 30.0F, 30.0F);
		this.rangedAttackTime = Math.max(this.rangedAttackTime - 1, 0);
		if(this.rangedAttackTime <= 0) {
			if(var3 <= var1 && var5) {
				this.doRangedAttack();
				this.rangedAttackTime = this.maxRangedAttackTime;
			}
		}
	}

	private void doRangedAttack() {
		if(this.rangedAttackID == 1) {
			EntityArrow var1 = new EntityArrow(this.worldObj, this.entityHost, this.attackTarget, 1.6F, 12.0F);
			this.worldObj.playSoundAtEntity(this.entityHost, "random.bow", 1.0F, 1.0F / (this.entityHost.getRNG().nextFloat() * 0.4F + 0.8F));
			this.worldObj.spawnEntityInWorld(var1);
		} else if(this.rangedAttackID == 2) {
			EntitySnowball var9 = new EntitySnowball(this.worldObj, this.entityHost);
			double var2 = this.attackTarget.posX - this.entityHost.posX;
			double var4 = this.attackTarget.posY + (double)this.attackTarget.getEyeHeight() - (double)1.1F - var9.posY;
			double var6 = this.attackTarget.posZ - this.entityHost.posZ;
			float var8 = MathHelper.sqrt_double(var2 * var2 + var6 * var6) * 0.2F;
			var9.setThrowableHeading(var2, var4 + (double)var8, var6, 1.6F, 12.0F);
			this.worldObj.playSoundAtEntity(this.entityHost, "random.bow", 1.0F, 1.0F / (this.entityHost.getRNG().nextFloat() * 0.4F + 0.8F));
			this.worldObj.spawnEntityInWorld(var9);
		}

	}
}
