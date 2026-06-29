package net.minecraft.src;

public class EntityLookHelper {
	private EntityLiving entity;
	private float field_46149_b;
	private float field_46150_c;
	private boolean field_46147_d = false;
	private double posX;
	private double posY;
	private double posZ;

	public EntityLookHelper(EntityLiving var1) {
		this.entity = var1;
	}

	public void setLookPositionWithEntity(Entity var1, float var2, float var3) {
		this.posX = var1.posX;
		if(var1 instanceof EntityLiving) {
			this.posY = var1.posY + (double)((EntityLiving)var1).getEyeHeight();
		} else {
			this.posY = (var1.boundingBox.minY + var1.boundingBox.maxY) / 2.0D;
		}

		this.posZ = var1.posZ;
		this.field_46149_b = var2;
		this.field_46150_c = var3;
		this.field_46147_d = true;
	}

	public void setLookPosition(double var1, double var3, double var5, float var7, float var8) {
		this.posX = var1;
		this.posY = var3;
		this.posZ = var5;
		this.field_46149_b = var7;
		this.field_46150_c = var8;
		this.field_46147_d = true;
	}

	public void onUpdateLook() {
		this.entity.rotationPitch = 0.0F;
		if(this.field_46147_d) {
			this.field_46147_d = false;
			double var1 = this.posX - this.entity.posX;
			double var3 = this.posY - (this.entity.posY + (double)this.entity.getEyeHeight());
			double var5 = this.posZ - this.entity.posZ;
			double var7 = (double)MathHelper.sqrt_double(var1 * var1 + var5 * var5);
			float var9 = (float)(Math.atan2(var5, var1) * 180.0D / (double)((float)Math.PI)) - 90.0F;
			float var10 = (float)(-(Math.atan2(var3, var7) * 180.0D / (double)((float)Math.PI)));
			this.entity.rotationPitch = this.updateRotation(this.entity.rotationPitch, var10, this.field_46150_c);
			this.entity.rotationYawHead = this.updateRotation(this.entity.rotationYawHead, var9, this.field_46149_b);
		} else {
			this.entity.rotationYawHead = this.updateRotation(this.entity.rotationYawHead, this.entity.renderYawOffset, 10.0F);
		}

		float var11;
		for(var11 = this.entity.rotationYawHead - this.entity.renderYawOffset; var11 < -180.0F; var11 += 360.0F) {
		}

		while(var11 >= 180.0F) {
			var11 -= 360.0F;
		}

		if(!this.entity.getNavigator().noPath()) {
			if(var11 < -75.0F) {
				this.entity.rotationYawHead = this.entity.renderYawOffset - 75.0F;
			}

			if(var11 > 75.0F) {
				this.entity.rotationYawHead = this.entity.renderYawOffset + 75.0F;
			}
		}

	}

	private float updateRotation(float var1, float var2, float var3) {
		float var4;
		for(var4 = var2 - var1; var4 < -180.0F; var4 += 360.0F) {
		}

		while(var4 >= 180.0F) {
			var4 -= 360.0F;
		}

		if(var4 > var3) {
			var4 = var3;
		}

		if(var4 < -var3) {
			var4 = -var3;
		}

		return var1 + var4;
	}
}
