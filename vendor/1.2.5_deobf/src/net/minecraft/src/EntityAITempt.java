package net.minecraft.src;

public class EntityAITempt extends EntityAIBase {
	private EntityCreature temptedEntity;
	private float field_48275_b;
	private double field_48276_c;
	private double field_48273_d;
	private double field_48274_e;
	private double field_48271_f;
	private double field_48272_g;
	private EntityPlayer temptingPlayer;
	private int delayTemptCounter = 0;
	private boolean field_48280_j;
	private int breedingFood;
	private boolean scaredByPlayerMovement;
	private boolean field_48279_m;

	public EntityAITempt(EntityCreature var1, float var2, int var3, boolean var4) {
		this.temptedEntity = var1;
		this.field_48275_b = var2;
		this.breedingFood = var3;
		this.scaredByPlayerMovement = var4;
		this.setMutexBits(3);
	}

	public boolean shouldExecute() {
		if(this.delayTemptCounter > 0) {
			--this.delayTemptCounter;
			return false;
		} else {
			this.temptingPlayer = this.temptedEntity.worldObj.getClosestPlayerToEntity(this.temptedEntity, 10.0D);
			if(this.temptingPlayer == null) {
				return false;
			} else {
				ItemStack var1 = this.temptingPlayer.getCurrentEquippedItem();
				return var1 == null ? false : var1.itemID == this.breedingFood;
			}
		}
	}

	public boolean continueExecuting() {
		if(this.scaredByPlayerMovement) {
			if(this.temptedEntity.getDistanceSqToEntity(this.temptingPlayer) < 36.0D) {
				if(this.temptingPlayer.getDistanceSq(this.field_48276_c, this.field_48273_d, this.field_48274_e) > 0.1D * 0.1D) {
					return false;
				}

				if(Math.abs((double)this.temptingPlayer.rotationPitch - this.field_48271_f) > 5.0D || Math.abs((double)this.temptingPlayer.rotationYaw - this.field_48272_g) > 5.0D) {
					return false;
				}
			} else {
				this.field_48276_c = this.temptingPlayer.posX;
				this.field_48273_d = this.temptingPlayer.posY;
				this.field_48274_e = this.temptingPlayer.posZ;
			}

			this.field_48271_f = (double)this.temptingPlayer.rotationPitch;
			this.field_48272_g = (double)this.temptingPlayer.rotationYaw;
		}

		return this.shouldExecute();
	}

	public void startExecuting() {
		this.field_48276_c = this.temptingPlayer.posX;
		this.field_48273_d = this.temptingPlayer.posY;
		this.field_48274_e = this.temptingPlayer.posZ;
		this.field_48280_j = true;
		this.field_48279_m = this.temptedEntity.getNavigator().func_48658_a();
		this.temptedEntity.getNavigator().func_48664_a(false);
	}

	public void resetTask() {
		this.temptingPlayer = null;
		this.temptedEntity.getNavigator().clearPathEntity();
		this.delayTemptCounter = 100;
		this.field_48280_j = false;
		this.temptedEntity.getNavigator().func_48664_a(this.field_48279_m);
	}

	public void updateTask() {
		this.temptedEntity.getLookHelper().setLookPositionWithEntity(this.temptingPlayer, 30.0F, (float)this.temptedEntity.getVerticalFaceSpeed());
		if(this.temptedEntity.getDistanceSqToEntity(this.temptingPlayer) < 6.25D) {
			this.temptedEntity.getNavigator().clearPathEntity();
		} else {
			this.temptedEntity.getNavigator().func_48667_a(this.temptingPlayer, this.field_48275_b);
		}

	}

	public boolean func_48270_h() {
		return this.field_48280_j;
	}
}
