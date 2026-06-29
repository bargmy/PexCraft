package net.minecraft.src;

public class ModelVillager extends ModelBase {
	public ModelRenderer field_40340_a;
	public ModelRenderer field_40338_b;
	public ModelRenderer field_40339_c;
	public ModelRenderer field_40336_d;
	public ModelRenderer field_40337_e;

	public ModelVillager(float var1) {
		this(var1, 0.0F);
	}

	public ModelVillager(float var1, float var2) {
		byte var3 = 64;
		byte var4 = 64;
		this.field_40340_a = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_40340_a.setRotationPoint(0.0F, 0.0F + var2, 0.0F);
		this.field_40340_a.setTextureOffset(0, 0).addBox(-4.0F, -10.0F, -4.0F, 8, 10, 8, var1);
		this.field_40340_a.setTextureOffset(24, 0).addBox(-1.0F, -3.0F, -6.0F, 2, 4, 2, var1);
		this.field_40338_b = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_40338_b.setRotationPoint(0.0F, 0.0F + var2, 0.0F);
		this.field_40338_b.setTextureOffset(16, 20).addBox(-4.0F, 0.0F, -3.0F, 8, 12, 6, var1);
		this.field_40338_b.setTextureOffset(0, 38).addBox(-4.0F, 0.0F, -3.0F, 8, 18, 6, var1 + 0.5F);
		this.field_40339_c = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_40339_c.setRotationPoint(0.0F, 0.0F + var2 + 2.0F, 0.0F);
		this.field_40339_c.setTextureOffset(44, 22).addBox(-8.0F, -2.0F, -2.0F, 4, 8, 4, var1);
		this.field_40339_c.setTextureOffset(44, 22).addBox(4.0F, -2.0F, -2.0F, 4, 8, 4, var1);
		this.field_40339_c.setTextureOffset(40, 38).addBox(-4.0F, 2.0F, -2.0F, 8, 4, 4, var1);
		this.field_40336_d = (new ModelRenderer(this, 0, 22)).setTextureSize(var3, var4);
		this.field_40336_d.setRotationPoint(-2.0F, 12.0F + var2, 0.0F);
		this.field_40336_d.addBox(-2.0F, 0.0F, -2.0F, 4, 12, 4, var1);
		this.field_40337_e = (new ModelRenderer(this, 0, 22)).setTextureSize(var3, var4);
		this.field_40337_e.mirror = true;
		this.field_40337_e.setRotationPoint(2.0F, 12.0F + var2, 0.0F);
		this.field_40337_e.addBox(-2.0F, 0.0F, -2.0F, 4, 12, 4, var1);
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		this.field_40340_a.render(var7);
		this.field_40338_b.render(var7);
		this.field_40336_d.render(var7);
		this.field_40337_e.render(var7);
		this.field_40339_c.render(var7);
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		this.field_40340_a.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		this.field_40340_a.rotateAngleX = var5 / (180.0F / (float)Math.PI);
		this.field_40339_c.rotationPointY = 3.0F;
		this.field_40339_c.rotationPointZ = -1.0F;
		this.field_40339_c.rotateAngleX = -(12.0F / 16.0F);
		this.field_40336_d.rotateAngleX = MathHelper.cos(var1 * 0.6662F) * 1.4F * var2 * 0.5F;
		this.field_40337_e.rotateAngleX = MathHelper.cos(var1 * 0.6662F + (float)Math.PI) * 1.4F * var2 * 0.5F;
		this.field_40336_d.rotateAngleY = 0.0F;
		this.field_40337_e.rotateAngleY = 0.0F;
	}
}
