package net.minecraft.src;

public class ModelBlaze extends ModelBase {
	private ModelRenderer[] field_40323_a = new ModelRenderer[12];
	private ModelRenderer field_40322_b;

	public ModelBlaze() {
		for(int var1 = 0; var1 < this.field_40323_a.length; ++var1) {
			this.field_40323_a[var1] = new ModelRenderer(this, 0, 16);
			this.field_40323_a[var1].addBox(0.0F, 0.0F, 0.0F, 2, 8, 2);
		}

		this.field_40322_b = new ModelRenderer(this, 0, 0);
		this.field_40322_b.addBox(-4.0F, -4.0F, -4.0F, 8, 8, 8);
	}

	public int func_40321_a() {
		return 8;
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		this.field_40322_b.render(var7);

		for(int var8 = 0; var8 < this.field_40323_a.length; ++var8) {
			this.field_40323_a[var8].render(var7);
		}

	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		float var7 = var3 * (float)Math.PI * -0.1F;

		int var8;
		for(var8 = 0; var8 < 4; ++var8) {
			this.field_40323_a[var8].rotationPointY = -2.0F + MathHelper.cos(((float)(var8 * 2) + var3) * 0.25F);
			this.field_40323_a[var8].rotationPointX = MathHelper.cos(var7) * 9.0F;
			this.field_40323_a[var8].rotationPointZ = MathHelper.sin(var7) * 9.0F;
			var7 += (float)Math.PI * 0.5F;
		}

		var7 = (float)Math.PI * 0.25F + var3 * (float)Math.PI * 0.03F;

		for(var8 = 4; var8 < 8; ++var8) {
			this.field_40323_a[var8].rotationPointY = 2.0F + MathHelper.cos(((float)(var8 * 2) + var3) * 0.25F);
			this.field_40323_a[var8].rotationPointX = MathHelper.cos(var7) * 7.0F;
			this.field_40323_a[var8].rotationPointZ = MathHelper.sin(var7) * 7.0F;
			var7 += (float)Math.PI * 0.5F;
		}

		var7 = (float)Math.PI * 0.15F + var3 * (float)Math.PI * -0.05F;

		for(var8 = 8; var8 < 12; ++var8) {
			this.field_40323_a[var8].rotationPointY = 11.0F + MathHelper.cos(((float)var8 * 1.5F + var3) * 0.5F);
			this.field_40323_a[var8].rotationPointX = MathHelper.cos(var7) * 5.0F;
			this.field_40323_a[var8].rotationPointZ = MathHelper.sin(var7) * 5.0F;
			var7 += (float)Math.PI * 0.5F;
		}

		this.field_40322_b.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		this.field_40322_b.rotateAngleX = var5 / (180.0F / (float)Math.PI);
	}
}
