package net.minecraft.src;

public class EntityAILeapAtTarget extends EntityAIBase {
	EntityLiving leaper;
	EntityLiving leapTarget;
	float leapMotionY;

	public EntityAILeapAtTarget(EntityLiving var1, float var2) {
		this.leaper = var1;
		this.leapMotionY = var2;
		this.setMutexBits(5);
	}

	public boolean shouldExecute() {
		this.leapTarget = this.leaper.getAttackTarget();
		if(this.leapTarget == null) {
			return false;
		} else {
			double var1 = this.leaper.getDistanceSqToEntity(this.leapTarget);
			return var1 >= 4.0D && var1 <= 16.0D ? (!this.leaper.onGround ? false : this.leaper.getRNG().nextInt(5) == 0) : false;
		}
	}

	public boolean continueExecuting() {
		return !this.leaper.onGround;
	}

	public void startExecuting() {
		double var1 = this.leapTarget.posX - this.leaper.posX;
		double var3 = this.leapTarget.posZ - this.leaper.posZ;
		float var5 = MathHelper.sqrt_double(var1 * var1 + var3 * var3);
		this.leaper.motionX += var1 / (double)var5 * 0.5D * (double)0.8F + this.leaper.motionX * (double)0.2F;
		this.leaper.motionZ += var3 / (double)var5 * 0.5D * (double)0.8F + this.leaper.motionZ * (double)0.2F;
		this.leaper.motionY = (double)this.leapMotionY;
	}
}
