package net.minecraft.src;

public class EntityAIOwnerHurtByTarget extends EntityAITarget {
	EntityTameable field_48394_a;
	EntityLiving field_48393_b;

	public EntityAIOwnerHurtByTarget(EntityTameable var1) {
		super(var1, 32.0F, false);
		this.field_48394_a = var1;
		this.setMutexBits(1);
	}

	public boolean shouldExecute() {
		if(!this.field_48394_a.isTamed()) {
			return false;
		} else {
			EntityLiving var1 = this.field_48394_a.getOwner();
			if(var1 == null) {
				return false;
			} else {
				this.field_48393_b = var1.getAITarget();
				return this.func_48376_a(this.field_48393_b, false);
			}
		}
	}

	public void startExecuting() {
		this.taskOwner.setAttackTarget(this.field_48393_b);
		super.startExecuting();
	}
}
