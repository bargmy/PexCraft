package net.minecraft.src;

public abstract class EntityAIDoorInteract extends EntityAIBase {
	protected EntityLiving theEntity;
	protected int entityPosX;
	protected int entityPosY;
	protected int entityPosZ;
	protected BlockDoor targetDoor;
	boolean field_48319_f;
	float field_48320_g;
	float field_48326_h;

	public EntityAIDoorInteract(EntityLiving var1) {
		this.theEntity = var1;
	}

	public boolean shouldExecute() {
		if(!this.theEntity.isCollidedHorizontally) {
			return false;
		} else {
			PathNavigate var1 = this.theEntity.getNavigator();
			PathEntity var2 = var1.getPath();
			if(var2 != null && !var2.isFinished() && var1.func_48665_b()) {
				for(int var3 = 0; var3 < Math.min(var2.getCurrentPathIndex() + 2, var2.getCurrentPathLength()); ++var3) {
					PathPoint var4 = var2.getPathPointFromIndex(var3);
					this.entityPosX = var4.xCoord;
					this.entityPosY = var4.yCoord + 1;
					this.entityPosZ = var4.zCoord;
					if(this.theEntity.getDistanceSq((double)this.entityPosX, this.theEntity.posY, (double)this.entityPosZ) <= 2.25D) {
						this.targetDoor = this.func_48318_a(this.entityPosX, this.entityPosY, this.entityPosZ);
						if(this.targetDoor != null) {
							return true;
						}
					}
				}

				this.entityPosX = MathHelper.floor_double(this.theEntity.posX);
				this.entityPosY = MathHelper.floor_double(this.theEntity.posY + 1.0D);
				this.entityPosZ = MathHelper.floor_double(this.theEntity.posZ);
				this.targetDoor = this.func_48318_a(this.entityPosX, this.entityPosY, this.entityPosZ);
				return this.targetDoor != null;
			} else {
				return false;
			}
		}
	}

	public boolean continueExecuting() {
		return !this.field_48319_f;
	}

	public void startExecuting() {
		this.field_48319_f = false;
		this.field_48320_g = (float)((double)((float)this.entityPosX + 0.5F) - this.theEntity.posX);
		this.field_48326_h = (float)((double)((float)this.entityPosZ + 0.5F) - this.theEntity.posZ);
	}

	public void updateTask() {
		float var1 = (float)((double)((float)this.entityPosX + 0.5F) - this.theEntity.posX);
		float var2 = (float)((double)((float)this.entityPosZ + 0.5F) - this.theEntity.posZ);
		float var3 = this.field_48320_g * var1 + this.field_48326_h * var2;
		if(var3 < 0.0F) {
			this.field_48319_f = true;
		}

	}

	private BlockDoor func_48318_a(int var1, int var2, int var3) {
		int var4 = this.theEntity.worldObj.getBlockId(var1, var2, var3);
		if(var4 != Block.doorWood.blockID) {
			return null;
		} else {
			BlockDoor var5 = (BlockDoor)Block.blocksList[var4];
			return var5;
		}
	}
}
