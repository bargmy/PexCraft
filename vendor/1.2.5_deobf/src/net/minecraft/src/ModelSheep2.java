package net.minecraft.src;

public class ModelSheep2 extends ModelQuadruped {
	private float field_44017_o;

	public ModelSheep2() {
		super(12, 0.0F);
		this.head = new ModelRenderer(this, 0, 0);
		this.head.addBox(-3.0F, -4.0F, -6.0F, 6, 6, 8, 0.0F);
		this.head.setRotationPoint(0.0F, 6.0F, -8.0F);
		this.body = new ModelRenderer(this, 28, 8);
		this.body.addBox(-4.0F, -10.0F, -7.0F, 8, 16, 6, 0.0F);
		this.body.setRotationPoint(0.0F, 5.0F, 2.0F);
	}

	public void setLivingAnimations(EntityLiving var1, float var2, float var3, float var4) {
		super.setLivingAnimations(var1, var2, var3, var4);
		this.head.rotationPointY = 6.0F + ((EntitySheep)var1).func_44003_c(var4) * 9.0F;
		this.field_44017_o = ((EntitySheep)var1).func_44002_d(var4);
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		super.setRotationAngles(var1, var2, var3, var4, var5, var6);
		this.head.rotateAngleX = this.field_44017_o;
	}
}
