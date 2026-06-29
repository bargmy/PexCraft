package net.minecraft.src;

public class EntityAISit extends EntityAIBase {
	private EntityTameable theEntity;
	private boolean field_48408_b = false;

	public EntityAISit(EntityTameable var1) {
		this.theEntity = var1;
		this.setMutexBits(5);
	}

	public boolean shouldExecute() {
		if(!this.theEntity.isTamed()) {
			return false;
		} else if(this.theEntity.isInWater()) {
			return false;
		} else if(!this.theEntity.onGround) {
			return false;
		} else {
			EntityLiving var1 = this.theEntity.getOwner();
			return var1 == null ? true : (this.theEntity.getDistanceSqToEntity(var1) < 144.0D && var1.getAITarget() != null ? false : this.field_48408_b);
		}
	}

	public void startExecuting() {
		this.theEntity.getNavigator().clearPathEntity();
		this.theEntity.func_48140_f(true);
	}

	public void resetTask() {
		this.theEntity.func_48140_f(false);
	}

	public void func_48407_a(boolean var1) {
		this.field_48408_b = var1;
	}
}
