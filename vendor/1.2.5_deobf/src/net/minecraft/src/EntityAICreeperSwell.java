package net.minecraft.src;

public class EntityAICreeperSwell extends EntityAIBase {
	EntityCreeper swellingCreeper;
	EntityLiving creeperAttackTarget;

	public EntityAICreeperSwell(EntityCreeper var1) {
		this.swellingCreeper = var1;
		this.setMutexBits(1);
	}

	public boolean shouldExecute() {
		EntityLiving var1 = this.swellingCreeper.getAttackTarget();
		return this.swellingCreeper.getCreeperState() > 0 || var1 != null && this.swellingCreeper.getDistanceSqToEntity(var1) < 9.0D;
	}

	public void startExecuting() {
		this.swellingCreeper.getNavigator().clearPathEntity();
		this.creeperAttackTarget = this.swellingCreeper.getAttackTarget();
	}

	public void resetTask() {
		this.creeperAttackTarget = null;
	}

	public void updateTask() {
		if(this.creeperAttackTarget == null) {
			this.swellingCreeper.setCreeperState(-1);
		} else if(this.swellingCreeper.getDistanceSqToEntity(this.creeperAttackTarget) > 49.0D) {
			this.swellingCreeper.setCreeperState(-1);
		} else if(!this.swellingCreeper.func_48090_aM().canSee(this.creeperAttackTarget)) {
			this.swellingCreeper.setCreeperState(-1);
		} else {
			this.swellingCreeper.setCreeperState(1);
		}
	}
}
