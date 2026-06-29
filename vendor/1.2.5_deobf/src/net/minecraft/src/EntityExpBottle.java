package net.minecraft.src;

public class EntityExpBottle extends EntityThrowable {
	public EntityExpBottle(World var1) {
		super(var1);
	}

	public EntityExpBottle(World var1, EntityLiving var2) {
		super(var1, var2);
	}

	public EntityExpBottle(World var1, double var2, double var4, double var6) {
		super(var1, var2, var4, var6);
	}

	protected float func_40075_e() {
		return 0.07F;
	}

	protected float func_40077_c() {
		return 0.7F;
	}

	protected float func_40074_d() {
		return -20.0F;
	}

	protected void onImpact(MovingObjectPosition var1) {
		if(!this.worldObj.isRemote) {
			this.worldObj.playAuxSFX(2002, (int)Math.round(this.posX), (int)Math.round(this.posY), (int)Math.round(this.posZ), 0);
			int var2 = 3 + this.worldObj.rand.nextInt(5) + this.worldObj.rand.nextInt(5);

			while(var2 > 0) {
				int var3 = EntityXPOrb.getXPSplit(var2);
				var2 -= var3;
				this.worldObj.spawnEntityInWorld(new EntityXPOrb(this.worldObj, this.posX, this.posY, this.posZ, var3));
			}

			this.setDead();
		}

	}
}
