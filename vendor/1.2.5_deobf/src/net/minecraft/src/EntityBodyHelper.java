package net.minecraft.src;

public class EntityBodyHelper {
	private EntityLiving field_48654_a;
	private int field_48652_b = 0;
	private float field_48653_c = 0.0F;

	public EntityBodyHelper(EntityLiving var1) {
		this.field_48654_a = var1;
	}

	public void func_48650_a() {
		double var1 = this.field_48654_a.posX - this.field_48654_a.prevPosX;
		double var3 = this.field_48654_a.posZ - this.field_48654_a.prevPosZ;
		if(var1 * var1 + var3 * var3 > (double)2.5000003E-7F) {
			this.field_48654_a.renderYawOffset = this.field_48654_a.rotationYaw;
			this.field_48654_a.rotationYawHead = this.func_48651_a(this.field_48654_a.renderYawOffset, this.field_48654_a.rotationYawHead, 75.0F);
			this.field_48653_c = this.field_48654_a.rotationYawHead;
			this.field_48652_b = 0;
		} else {
			float var5 = 75.0F;
			if(Math.abs(this.field_48654_a.rotationYawHead - this.field_48653_c) > 15.0F) {
				this.field_48652_b = 0;
				this.field_48653_c = this.field_48654_a.rotationYawHead;
			} else {
				++this.field_48652_b;
				if(this.field_48652_b > 10) {
					var5 = Math.max(1.0F - (float)(this.field_48652_b - 10) / 10.0F, 0.0F) * 75.0F;
				}
			}

			this.field_48654_a.renderYawOffset = this.func_48651_a(this.field_48654_a.rotationYawHead, this.field_48654_a.renderYawOffset, var5);
		}
	}

	private float func_48651_a(float var1, float var2, float var3) {
		float var4;
		for(var4 = var1 - var2; var4 < -180.0F; var4 += 360.0F) {
		}

		while(var4 >= 180.0F) {
			var4 -= 360.0F;
		}

		if(var4 < -var3) {
			var4 = -var3;
		}

		if(var4 >= var3) {
			var4 = var3;
		}

		return var1 - var4;
	}
}
