package net.minecraft.src;

public class EntityAIAttackOnCollide extends EntityAIBase {
	World worldObj;
	EntityLiving attacker;
	EntityLiving entityTarget;
	int field_46091_d;
	float field_48266_e;
	boolean field_48264_f;
	PathEntity field_48265_g;
	Class classTarget;
	private int field_48269_i;

	public EntityAIAttackOnCollide(EntityLiving var1, Class var2, float var3, boolean var4) {
		this(var1, var3, var4);
		this.classTarget = var2;
	}

	public EntityAIAttackOnCollide(EntityLiving var1, float var2, boolean var3) {
		this.field_46091_d = 0;
		this.attacker = var1;
		this.worldObj = var1.worldObj;
		this.field_48266_e = var2;
		this.field_48264_f = var3;
		this.setMutexBits(3);
	}

	public boolean shouldExecute() {
		EntityLiving var1 = this.attacker.getAttackTarget();
		if(var1 == null) {
			return false;
		} else if(this.classTarget != null && !this.classTarget.isAssignableFrom(var1.getClass())) {
			return false;
		} else {
			this.entityTarget = var1;
			this.field_48265_g = this.attacker.getNavigator().func_48679_a(this.entityTarget);
			return this.field_48265_g != null;
		}
	}

	public boolean continueExecuting() {
		EntityLiving var1 = this.attacker.getAttackTarget();
		return var1 == null ? false : (!this.entityTarget.isEntityAlive() ? false : (!this.field_48264_f ? !this.attacker.getNavigator().noPath() : this.attacker.isWithinHomeDistance(MathHelper.floor_double(this.entityTarget.posX), MathHelper.floor_double(this.entityTarget.posY), MathHelper.floor_double(this.entityTarget.posZ))));
	}

	public void startExecuting() {
		this.attacker.getNavigator().setPath(this.field_48265_g, this.field_48266_e);
		this.field_48269_i = 0;
	}

	public void resetTask() {
		this.entityTarget = null;
		this.attacker.getNavigator().clearPathEntity();
	}

	public void updateTask() {
		this.attacker.getLookHelper().setLookPositionWithEntity(this.entityTarget, 30.0F, 30.0F);
		if((this.field_48264_f || this.attacker.func_48090_aM().canSee(this.entityTarget)) && --this.field_48269_i <= 0) {
			this.field_48269_i = 4 + this.attacker.getRNG().nextInt(7);
			this.attacker.getNavigator().func_48667_a(this.entityTarget, this.field_48266_e);
		}

		this.field_46091_d = Math.max(this.field_46091_d - 1, 0);
		double var1 = (double)(this.attacker.width * 2.0F * this.attacker.width * 2.0F);
		if(this.attacker.getDistanceSq(this.entityTarget.posX, this.entityTarget.boundingBox.minY, this.entityTarget.posZ) <= var1) {
			if(this.field_46091_d <= 0) {
				this.field_46091_d = 20;
				this.attacker.attackEntityAsMob(this.entityTarget);
			}
		}
	}
}
