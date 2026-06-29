package net.minecraft.src;

public class EntityAISwimming extends EntityAIBase {
	private EntityLiving theEntity;

	public EntityAISwimming(EntityLiving var1) {
		this.theEntity = var1;
		this.setMutexBits(4);
		var1.getNavigator().func_48669_e(true);
	}

	public boolean shouldExecute() {
		return this.theEntity.isInWater() || this.theEntity.handleLavaMovement();
	}

	public void updateTask() {
		if(this.theEntity.getRNG().nextFloat() < 0.8F) {
			this.theEntity.getJumpHelper().setJumping();
		}

	}
}
