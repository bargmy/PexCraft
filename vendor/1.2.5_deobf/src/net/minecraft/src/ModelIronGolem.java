package net.minecraft.src;

public class ModelIronGolem extends ModelBase {
	public ModelRenderer field_48234_a;
	public ModelRenderer field_48232_b;
	public ModelRenderer field_48233_c;
	public ModelRenderer field_48230_d;
	public ModelRenderer field_48231_e;
	public ModelRenderer field_48229_f;

	public ModelIronGolem() {
		this(0.0F);
	}

	public ModelIronGolem(float var1) {
		this(var1, -7.0F);
	}

	public ModelIronGolem(float var1, float var2) {
		short var3 = 128;
		short var4 = 128;
		this.field_48234_a = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_48234_a.setRotationPoint(0.0F, 0.0F + var2, -2.0F);
		this.field_48234_a.setTextureOffset(0, 0).addBox(-4.0F, -12.0F, -5.5F, 8, 10, 8, var1);
		this.field_48234_a.setTextureOffset(24, 0).addBox(-1.0F, -5.0F, -7.5F, 2, 4, 2, var1);
		this.field_48232_b = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_48232_b.setRotationPoint(0.0F, 0.0F + var2, 0.0F);
		this.field_48232_b.setTextureOffset(0, 40).addBox(-9.0F, -2.0F, -6.0F, 18, 12, 11, var1);
		this.field_48232_b.setTextureOffset(0, 70).addBox(-4.5F, 10.0F, -3.0F, 9, 5, 6, var1 + 0.5F);
		this.field_48233_c = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_48233_c.setRotationPoint(0.0F, -7.0F, 0.0F);
		this.field_48233_c.setTextureOffset(60, 21).addBox(-13.0F, -2.5F, -3.0F, 4, 30, 6, var1);
		this.field_48230_d = (new ModelRenderer(this)).setTextureSize(var3, var4);
		this.field_48230_d.setRotationPoint(0.0F, -7.0F, 0.0F);
		this.field_48230_d.setTextureOffset(60, 58).addBox(9.0F, -2.5F, -3.0F, 4, 30, 6, var1);
		this.field_48231_e = (new ModelRenderer(this, 0, 22)).setTextureSize(var3, var4);
		this.field_48231_e.setRotationPoint(-4.0F, 18.0F + var2, 0.0F);
		this.field_48231_e.setTextureOffset(37, 0).addBox(-3.5F, -3.0F, -3.0F, 6, 16, 5, var1);
		this.field_48229_f = (new ModelRenderer(this, 0, 22)).setTextureSize(var3, var4);
		this.field_48229_f.mirror = true;
		this.field_48229_f.setTextureOffset(60, 0).setRotationPoint(5.0F, 18.0F + var2, 0.0F);
		this.field_48229_f.addBox(-3.5F, -3.0F, -3.0F, 6, 16, 5, var1);
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		this.field_48234_a.render(var7);
		this.field_48232_b.render(var7);
		this.field_48231_e.render(var7);
		this.field_48229_f.render(var7);
		this.field_48233_c.render(var7);
		this.field_48230_d.render(var7);
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		this.field_48234_a.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		this.field_48234_a.rotateAngleX = var5 / (180.0F / (float)Math.PI);
		this.field_48231_e.rotateAngleX = -1.5F * this.func_48228_a(var1, 13.0F) * var2;
		this.field_48229_f.rotateAngleX = 1.5F * this.func_48228_a(var1, 13.0F) * var2;
		this.field_48231_e.rotateAngleY = 0.0F;
		this.field_48229_f.rotateAngleY = 0.0F;
	}

	public void setLivingAnimations(EntityLiving var1, float var2, float var3, float var4) {
		EntityIronGolem var5 = (EntityIronGolem)var1;
		int var6 = var5.func_48114_ab();
		if(var6 > 0) {
			this.field_48233_c.rotateAngleX = -2.0F + 1.5F * this.func_48228_a((float)var6 - var4, 10.0F);
			this.field_48230_d.rotateAngleX = -2.0F + 1.5F * this.func_48228_a((float)var6 - var4, 10.0F);
		} else {
			int var7 = var5.func_48117_D_();
			if(var7 > 0) {
				this.field_48233_c.rotateAngleX = -0.8F + 0.025F * this.func_48228_a((float)var7, 70.0F);
				this.field_48230_d.rotateAngleX = 0.0F;
			} else {
				this.field_48233_c.rotateAngleX = (-0.2F + 1.5F * this.func_48228_a(var2, 13.0F)) * var3;
				this.field_48230_d.rotateAngleX = (-0.2F - 1.5F * this.func_48228_a(var2, 13.0F)) * var3;
			}
		}

	}

	private float func_48228_a(float var1, float var2) {
		return (Math.abs(var1 % var2 - var2 * 0.5F) - var2 * 0.25F) / (var2 * 0.25F);
	}
}
