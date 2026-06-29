package net.minecraft.src;

public abstract class EntityAITarget extends EntityAIBase {
	protected EntityLiving taskOwner;
	protected float field_48379_d;
	protected boolean field_48380_e;
	private boolean field_48383_a;
	private int field_48381_b;
	private int field_48377_f;
	private int field_48378_g;

	public EntityAITarget(EntityLiving var1, float var2, boolean var3) {
		this(var1, var2, var3, false);
	}

	public EntityAITarget(EntityLiving var1, float var2, boolean var3, boolean var4) {
		this.field_48381_b = 0;
		this.field_48377_f = 0;
		this.field_48378_g = 0;
		this.taskOwner = var1;
		this.field_48379_d = var2;
		this.field_48380_e = var3;
		this.field_48383_a = var4;
	}

	public boolean continueExecuting() {
		EntityLiving var1 = this.taskOwner.getAttackTarget();
		if(var1 == null) {
			return false;
		} else if(!var1.isEntityAlive()) {
			return false;
		} else if(this.taskOwner.getDistanceSqToEntity(var1) > (double)(this.field_48379_d * this.field_48379_d)) {
			return false;
		} else {
			if(this.field_48380_e) {
				if(!this.taskOwner.func_48090_aM().canSee(var1)) {
					if(++this.field_48378_g > 60) {
						return false;
					}
				} else {
					this.field_48378_g = 0;
				}
			}

			return true;
		}
	}

	public void startExecuting() {
		this.field_48381_b = 0;
		this.field_48377_f = 0;
		this.field_48378_g = 0;
	}

	public void resetTask() {
		this.taskOwner.setAttackTarget((EntityLiving)null);
	}

	protected boolean func_48376_a(EntityLiving var1, boolean var2) {
		if(var1 == null) {
			return false;
		} else if(var1 == this.taskOwner) {
			return false;
		} else if(!var1.isEntityAlive()) {
			return false;
		} else if(var1.boundingBox.maxY > this.taskOwner.boundingBox.minY && var1.boundingBox.minY < this.taskOwner.boundingBox.maxY) {
			if(!this.taskOwner.func_48100_a(var1.getClass())) {
				return false;
			} else {
				if(this.taskOwner instanceof EntityTameable && ((EntityTameable)this.taskOwner).isTamed()) {
					if(var1 instanceof EntityTameable && ((EntityTameable)var1).isTamed()) {
						return false;
					}

					if(var1 == ((EntityTameable)this.taskOwner).getOwner()) {
						return false;
					}
				} else if(var1 instanceof EntityPlayer && !var2 && ((EntityPlayer)var1).capabilities.disableDamage) {
					return false;
				}

				if(!this.taskOwner.isWithinHomeDistance(MathHelper.floor_double(var1.posX), MathHelper.floor_double(var1.posY), MathHelper.floor_double(var1.posZ))) {
					return false;
				} else if(this.field_48380_e && !this.taskOwner.func_48090_aM().canSee(var1)) {
					return false;
				} else {
					if(this.field_48383_a) {
						if(--this.field_48377_f <= 0) {
							this.field_48381_b = 0;
						}

						if(this.field_48381_b == 0) {
							this.field_48381_b = this.func_48375_a(var1) ? 1 : 2;
						}

						if(this.field_48381_b == 2) {
							return false;
						}
					}

					return true;
				}
			}
		} else {
			return false;
		}
	}

	private boolean func_48375_a(EntityLiving var1) {
		this.field_48377_f = 10 + this.taskOwner.getRNG().nextInt(5);
		PathEntity var2 = this.taskOwner.getNavigator().func_48679_a(var1);
		if(var2 == null) {
			return false;
		} else {
			PathPoint var3 = var2.getFinalPathPoint();
			if(var3 == null) {
				return false;
			} else {
				int var4 = var3.xCoord - MathHelper.floor_double(var1.posX);
				int var5 = var3.zCoord - MathHelper.floor_double(var1.posZ);
				return (double)(var4 * var4 + var5 * var5) <= 2.25D;
			}
		}
	}
}
