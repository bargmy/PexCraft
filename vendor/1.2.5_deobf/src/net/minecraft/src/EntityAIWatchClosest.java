package net.minecraft.src;

public class EntityAIWatchClosest extends EntityAIBase {
	private EntityLiving field_46105_a;
	private Entity closestEntity;
	private float field_46101_d;
	private int field_46102_e;
	private float field_48294_e;
	private Class field_48293_f;

	public EntityAIWatchClosest(EntityLiving var1, Class var2, float var3) {
		this.field_46105_a = var1;
		this.field_48293_f = var2;
		this.field_46101_d = var3;
		this.field_48294_e = 0.02F;
		this.setMutexBits(2);
	}

	public EntityAIWatchClosest(EntityLiving var1, Class var2, float var3, float var4) {
		this.field_46105_a = var1;
		this.field_48293_f = var2;
		this.field_46101_d = var3;
		this.field_48294_e = var4;
		this.setMutexBits(2);
	}

	public boolean shouldExecute() {
		if(this.field_46105_a.getRNG().nextFloat() >= this.field_48294_e) {
			return false;
		} else {
			if(this.field_48293_f == EntityPlayer.class) {
				this.closestEntity = this.field_46105_a.worldObj.getClosestPlayerToEntity(this.field_46105_a, (double)this.field_46101_d);
			} else {
				this.closestEntity = this.field_46105_a.worldObj.findNearestEntityWithinAABB(this.field_48293_f, this.field_46105_a.boundingBox.expand((double)this.field_46101_d, 3.0D, (double)this.field_46101_d), this.field_46105_a);
			}

			return this.closestEntity != null;
		}
	}

	public boolean continueExecuting() {
		return !this.closestEntity.isEntityAlive() ? false : (this.field_46105_a.getDistanceSqToEntity(this.closestEntity) > (double)(this.field_46101_d * this.field_46101_d) ? false : this.field_46102_e > 0);
	}

	public void startExecuting() {
		this.field_46102_e = 40 + this.field_46105_a.getRNG().nextInt(40);
	}

	public void resetTask() {
		this.closestEntity = null;
	}

	public void updateTask() {
		this.field_46105_a.getLookHelper().setLookPosition(this.closestEntity.posX, this.closestEntity.posY + (double)this.closestEntity.getEyeHeight(), this.closestEntity.posZ, 10.0F, (float)this.field_46105_a.getVerticalFaceSpeed());
		--this.field_46102_e;
	}
}
