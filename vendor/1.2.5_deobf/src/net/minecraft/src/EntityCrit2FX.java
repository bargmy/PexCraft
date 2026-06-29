package net.minecraft.src;

public class EntityCrit2FX extends EntityFX {
	private Entity field_35134_a;
	private int currentLife;
	private int maximumLife;
	private String particleName;

	public EntityCrit2FX(World var1, Entity var2) {
		this(var1, var2, "crit");
	}

	public EntityCrit2FX(World var1, Entity var2, String var3) {
		super(var1, var2.posX, var2.boundingBox.minY + (double)(var2.height / 2.0F), var2.posZ, var2.motionX, var2.motionY, var2.motionZ);
		this.currentLife = 0;
		this.maximumLife = 0;
		this.field_35134_a = var2;
		this.maximumLife = 3;
		this.particleName = var3;
		this.onUpdate();
	}

	public void renderParticle(Tessellator var1, float var2, float var3, float var4, float var5, float var6, float var7) {
	}

	public void onUpdate() {
		for(int var1 = 0; var1 < 16; ++var1) {
			double var2 = (double)(this.rand.nextFloat() * 2.0F - 1.0F);
			double var4 = (double)(this.rand.nextFloat() * 2.0F - 1.0F);
			double var6 = (double)(this.rand.nextFloat() * 2.0F - 1.0F);
			if(var2 * var2 + var4 * var4 + var6 * var6 <= 1.0D) {
				double var8 = this.field_35134_a.posX + var2 * (double)this.field_35134_a.width / 4.0D;
				double var10 = this.field_35134_a.boundingBox.minY + (double)(this.field_35134_a.height / 2.0F) + var4 * (double)this.field_35134_a.height / 4.0D;
				double var12 = this.field_35134_a.posZ + var6 * (double)this.field_35134_a.width / 4.0D;
				this.worldObj.spawnParticle(this.particleName, var8, var10, var12, var2, var4 + 0.2D, var6);
			}
		}

		++this.currentLife;
		if(this.currentLife >= this.maximumLife) {
			this.setDead();
		}

	}

	public int getFXLayer() {
		return 3;
	}
}
