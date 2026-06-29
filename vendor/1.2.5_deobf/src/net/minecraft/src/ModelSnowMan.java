package net.minecraft.src;

public class ModelSnowMan extends ModelBase {
	public ModelRenderer field_40306_a;
	public ModelRenderer field_40304_b;
	public ModelRenderer field_40305_c;
	public ModelRenderer field_40302_d;
	public ModelRenderer field_40303_e;

	public ModelSnowMan() {
		float var1 = 4.0F;
		float var2 = 0.0F;
		this.field_40305_c = (new ModelRenderer(this, 0, 0)).setTextureSize(64, 64);
		this.field_40305_c.addBox(-4.0F, -8.0F, -4.0F, 8, 8, 8, var2 - 0.5F);
		this.field_40305_c.setRotationPoint(0.0F, 0.0F + var1, 0.0F);
		this.field_40302_d = (new ModelRenderer(this, 32, 0)).setTextureSize(64, 64);
		this.field_40302_d.addBox(-1.0F, 0.0F, -1.0F, 12, 2, 2, var2 - 0.5F);
		this.field_40302_d.setRotationPoint(0.0F, 0.0F + var1 + 9.0F - 7.0F, 0.0F);
		this.field_40303_e = (new ModelRenderer(this, 32, 0)).setTextureSize(64, 64);
		this.field_40303_e.addBox(-1.0F, 0.0F, -1.0F, 12, 2, 2, var2 - 0.5F);
		this.field_40303_e.setRotationPoint(0.0F, 0.0F + var1 + 9.0F - 7.0F, 0.0F);
		this.field_40306_a = (new ModelRenderer(this, 0, 16)).setTextureSize(64, 64);
		this.field_40306_a.addBox(-5.0F, -10.0F, -5.0F, 10, 10, 10, var2 - 0.5F);
		this.field_40306_a.setRotationPoint(0.0F, 0.0F + var1 + 9.0F, 0.0F);
		this.field_40304_b = (new ModelRenderer(this, 0, 36)).setTextureSize(64, 64);
		this.field_40304_b.addBox(-6.0F, -12.0F, -6.0F, 12, 12, 12, var2 - 0.5F);
		this.field_40304_b.setRotationPoint(0.0F, 0.0F + var1 + 20.0F, 0.0F);
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		super.setRotationAngles(var1, var2, var3, var4, var5, var6);
		this.field_40305_c.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		this.field_40305_c.rotateAngleX = var5 / (180.0F / (float)Math.PI);
		this.field_40306_a.rotateAngleY = var4 / (180.0F / (float)Math.PI) * 0.25F;
		float var7 = MathHelper.sin(this.field_40306_a.rotateAngleY);
		float var8 = MathHelper.cos(this.field_40306_a.rotateAngleY);
		this.field_40302_d.rotateAngleZ = 1.0F;
		this.field_40303_e.rotateAngleZ = -1.0F;
		this.field_40302_d.rotateAngleY = 0.0F + this.field_40306_a.rotateAngleY;
		this.field_40303_e.rotateAngleY = (float)Math.PI + this.field_40306_a.rotateAngleY;
		this.field_40302_d.rotationPointX = var8 * 5.0F;
		this.field_40302_d.rotationPointZ = -var7 * 5.0F;
		this.field_40303_e.rotationPointX = -var8 * 5.0F;
		this.field_40303_e.rotationPointZ = var7 * 5.0F;
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		this.field_40306_a.render(var7);
		this.field_40304_b.render(var7);
		this.field_40305_c.render(var7);
		this.field_40302_d.render(var7);
		this.field_40303_e.render(var7);
	}
}
