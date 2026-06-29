package net.minecraft.src;

public class EntityAIOpenDoor extends EntityAIDoorInteract {
	boolean field_48328_i;
	int field_48327_j;

	public EntityAIOpenDoor(EntityLiving var1, boolean var2) {
		super(var1);
		this.theEntity = var1;
		this.field_48328_i = var2;
	}

	public boolean continueExecuting() {
		return this.field_48328_i && this.field_48327_j > 0 && super.continueExecuting();
	}

	public void startExecuting() {
		this.field_48327_j = 20;
		this.targetDoor.onPoweredBlockChange(this.theEntity.worldObj, this.entityPosX, this.entityPosY, this.entityPosZ, true);
	}

	public void resetTask() {
		if(this.field_48328_i) {
			this.targetDoor.onPoweredBlockChange(this.theEntity.worldObj, this.entityPosX, this.entityPosY, this.entityPosZ, false);
		}

	}

	public void updateTask() {
		--this.field_48327_j;
		super.updateTask();
	}
}
