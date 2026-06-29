package net.minecraft.src;

public class EntityAIRestrictSun extends EntityAIBase {
	private EntityCreature theEntity;

	public EntityAIRestrictSun(EntityCreature var1) {
		this.theEntity = var1;
	}

	public boolean shouldExecute() {
		return this.theEntity.worldObj.isDaytime();
	}

	public void startExecuting() {
		this.theEntity.getNavigator().func_48680_d(true);
	}

	public void resetTask() {
		this.theEntity.getNavigator().func_48680_d(false);
	}
}
