package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class ModelWolf extends ModelBase {
	public ModelRenderer wolfHeadMain;
	public ModelRenderer wolfBody;
	public ModelRenderer wolfLeg1;
	public ModelRenderer wolfLeg2;
	public ModelRenderer wolfLeg3;
	public ModelRenderer wolfLeg4;
	ModelRenderer wolfTail;
	ModelRenderer wolfMane;

	public ModelWolf() {
		float var1 = 0.0F;
		float var2 = 13.5F;
		this.wolfHeadMain = new ModelRenderer(this, 0, 0);
		this.wolfHeadMain.addBox(-3.0F, -3.0F, -2.0F, 6, 6, 4, var1);
		this.wolfHeadMain.setRotationPoint(-1.0F, var2, -7.0F);
		this.wolfBody = new ModelRenderer(this, 18, 14);
		this.wolfBody.addBox(-4.0F, -2.0F, -3.0F, 6, 9, 6, var1);
		this.wolfBody.setRotationPoint(0.0F, 14.0F, 2.0F);
		this.wolfMane = new ModelRenderer(this, 21, 0);
		this.wolfMane.addBox(-4.0F, -3.0F, -3.0F, 8, 6, 7, var1);
		this.wolfMane.setRotationPoint(-1.0F, 14.0F, 2.0F);
		this.wolfLeg1 = new ModelRenderer(this, 0, 18);
		this.wolfLeg1.addBox(-1.0F, 0.0F, -1.0F, 2, 8, 2, var1);
		this.wolfLeg1.setRotationPoint(-2.5F, 16.0F, 7.0F);
		this.wolfLeg2 = new ModelRenderer(this, 0, 18);
		this.wolfLeg2.addBox(-1.0F, 0.0F, -1.0F, 2, 8, 2, var1);
		this.wolfLeg2.setRotationPoint(0.5F, 16.0F, 7.0F);
		this.wolfLeg3 = new ModelRenderer(this, 0, 18);
		this.wolfLeg3.addBox(-1.0F, 0.0F, -1.0F, 2, 8, 2, var1);
		this.wolfLeg3.setRotationPoint(-2.5F, 16.0F, -4.0F);
		this.wolfLeg4 = new ModelRenderer(this, 0, 18);
		this.wolfLeg4.addBox(-1.0F, 0.0F, -1.0F, 2, 8, 2, var1);
		this.wolfLeg4.setRotationPoint(0.5F, 16.0F, -4.0F);
		this.wolfTail = new ModelRenderer(this, 9, 18);
		this.wolfTail.addBox(-1.0F, 0.0F, -1.0F, 2, 8, 2, var1);
		this.wolfTail.setRotationPoint(-1.0F, 12.0F, 8.0F);
		this.wolfHeadMain.setTextureOffset(16, 14).addBox(-3.0F, -5.0F, 0.0F, 2, 2, 1, var1);
		this.wolfHeadMain.setTextureOffset(16, 14).addBox(1.0F, -5.0F, 0.0F, 2, 2, 1, var1);
		this.wolfHeadMain.setTextureOffset(0, 10).addBox(-1.5F, 0.0F, -5.0F, 3, 3, 4, var1);
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		super.render(var1, var2, var3, var4, var5, var6, var7);
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		if(this.isChild) {
			float var8 = 2.0F;
			GL11.glPushMatrix();
			GL11.glTranslatef(0.0F, 5.0F * var7, 2.0F * var7);
			this.wolfHeadMain.renderWithRotation(var7);
			GL11.glPopMatrix();
			GL11.glPushMatrix();
			GL11.glScalef(1.0F / var8, 1.0F / var8, 1.0F / var8);
			GL11.glTranslatef(0.0F, 24.0F * var7, 0.0F);
			this.wolfBody.render(var7);
			this.wolfLeg1.render(var7);
			this.wolfLeg2.render(var7);
			this.wolfLeg3.render(var7);
			this.wolfLeg4.render(var7);
			this.wolfTail.renderWithRotation(var7);
			this.wolfMane.render(var7);
			GL11.glPopMatrix();
		} else {
			this.wolfHeadMain.renderWithRotation(var7);
			this.wolfBody.render(var7);
			this.wolfLeg1.render(var7);
			this.wolfLeg2.render(var7);
			this.wolfLeg3.render(var7);
			this.wolfLeg4.render(var7);
			this.wolfTail.renderWithRotation(var7);
			this.wolfMane.render(var7);
		}

	}

	public void setLivingAnimations(EntityLiving var1, float var2, float var3, float var4) {
		EntityWolf var5 = (EntityWolf)var1;
		if(var5.isAngry()) {
			this.wolfTail.rotateAngleY = 0.0F;
		} else {
			this.wolfTail.rotateAngleY = MathHelper.cos(var2 * 0.6662F) * 1.4F * var3;
		}

		if(var5.isSitting()) {
			this.wolfMane.setRotationPoint(-1.0F, 16.0F, -3.0F);
			this.wolfMane.rotateAngleX = (float)Math.PI * 0.4F;
			this.wolfMane.rotateAngleY = 0.0F;
			this.wolfBody.setRotationPoint(0.0F, 18.0F, 0.0F);
			this.wolfBody.rotateAngleX = (float)Math.PI * 0.25F;
			this.wolfTail.setRotationPoint(-1.0F, 21.0F, 6.0F);
			this.wolfLeg1.setRotationPoint(-2.5F, 22.0F, 2.0F);
			this.wolfLeg1.rotateAngleX = (float)Math.PI * 3.0F / 2.0F;
			this.wolfLeg2.setRotationPoint(0.5F, 22.0F, 2.0F);
			this.wolfLeg2.rotateAngleX = (float)Math.PI * 3.0F / 2.0F;
			this.wolfLeg3.rotateAngleX = (float)Math.PI * 1.85F;
			this.wolfLeg3.setRotationPoint(-2.49F, 17.0F, -4.0F);
			this.wolfLeg4.rotateAngleX = (float)Math.PI * 1.85F;
			this.wolfLeg4.setRotationPoint(0.51F, 17.0F, -4.0F);
		} else {
			this.wolfBody.setRotationPoint(0.0F, 14.0F, 2.0F);
			this.wolfBody.rotateAngleX = (float)Math.PI * 0.5F;
			this.wolfMane.setRotationPoint(-1.0F, 14.0F, -3.0F);
			this.wolfMane.rotateAngleX = this.wolfBody.rotateAngleX;
			this.wolfTail.setRotationPoint(-1.0F, 12.0F, 8.0F);
			this.wolfLeg1.setRotationPoint(-2.5F, 16.0F, 7.0F);
			this.wolfLeg2.setRotationPoint(0.5F, 16.0F, 7.0F);
			this.wolfLeg3.setRotationPoint(-2.5F, 16.0F, -4.0F);
			this.wolfLeg4.setRotationPoint(0.5F, 16.0F, -4.0F);
			this.wolfLeg1.rotateAngleX = MathHelper.cos(var2 * 0.6662F) * 1.4F * var3;
			this.wolfLeg2.rotateAngleX = MathHelper.cos(var2 * 0.6662F + (float)Math.PI) * 1.4F * var3;
			this.wolfLeg3.rotateAngleX = MathHelper.cos(var2 * 0.6662F + (float)Math.PI) * 1.4F * var3;
			this.wolfLeg4.rotateAngleX = MathHelper.cos(var2 * 0.6662F) * 1.4F * var3;
		}

		this.wolfHeadMain.rotateAngleZ = var5.getInterestedAngle(var4) + var5.getShakeAngle(var4, 0.0F);
		this.wolfMane.rotateAngleZ = var5.getShakeAngle(var4, -0.08F);
		this.wolfBody.rotateAngleZ = var5.getShakeAngle(var4, -0.16F);
		this.wolfTail.rotateAngleZ = var5.getShakeAngle(var4, -0.2F);
		if(var5.getWolfShaking()) {
			float var6 = var5.getBrightness(var4) * var5.getShadingWhileShaking(var4);
			GL11.glColor3f(var6, var6, var6);
		}

	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		super.setRotationAngles(var1, var2, var3, var4, var5, var6);
		this.wolfHeadMain.rotateAngleX = var5 / (180.0F / (float)Math.PI);
		this.wolfHeadMain.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		this.wolfTail.rotateAngleX = var3;
	}
}
