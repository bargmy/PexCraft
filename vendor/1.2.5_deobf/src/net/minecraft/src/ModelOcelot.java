package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class ModelOcelot extends ModelBase {
	ModelRenderer field_48225_a;
	ModelRenderer field_48223_b;
	ModelRenderer field_48224_c;
	ModelRenderer field_48221_d;
	ModelRenderer field_48222_e;
	ModelRenderer field_48219_f;
	ModelRenderer field_48220_g;
	ModelRenderer field_48226_n;
	int field_48227_o = 1;

	public ModelOcelot() {
		this.setTextureOffset("head.main", 0, 0);
		this.setTextureOffset("head.nose", 0, 24);
		this.setTextureOffset("head.ear1", 0, 10);
		this.setTextureOffset("head.ear2", 6, 10);
		this.field_48220_g = new ModelRenderer(this, "head");
		this.field_48220_g.addBox("main", -2.5F, -2.0F, -3.0F, 5, 4, 5);
		this.field_48220_g.addBox("nose", -1.5F, 0.0F, -4.0F, 3, 2, 2);
		this.field_48220_g.addBox("ear1", -2.0F, -3.0F, 0.0F, 1, 1, 2);
		this.field_48220_g.addBox("ear2", 1.0F, -3.0F, 0.0F, 1, 1, 2);
		this.field_48220_g.setRotationPoint(0.0F, 15.0F, -9.0F);
		this.field_48226_n = new ModelRenderer(this, 20, 0);
		this.field_48226_n.addBox(-2.0F, 3.0F, -8.0F, 4, 16, 6, 0.0F);
		this.field_48226_n.setRotationPoint(0.0F, 12.0F, -10.0F);
		this.field_48222_e = new ModelRenderer(this, 0, 15);
		this.field_48222_e.addBox(-0.5F, 0.0F, 0.0F, 1, 8, 1);
		this.field_48222_e.rotateAngleX = 0.9F;
		this.field_48222_e.setRotationPoint(0.0F, 15.0F, 8.0F);
		this.field_48219_f = new ModelRenderer(this, 4, 15);
		this.field_48219_f.addBox(-0.5F, 0.0F, 0.0F, 1, 8, 1);
		this.field_48219_f.setRotationPoint(0.0F, 20.0F, 14.0F);
		this.field_48225_a = new ModelRenderer(this, 8, 13);
		this.field_48225_a.addBox(-1.0F, 0.0F, 1.0F, 2, 6, 2);
		this.field_48225_a.setRotationPoint(1.1F, 18.0F, 5.0F);
		this.field_48223_b = new ModelRenderer(this, 8, 13);
		this.field_48223_b.addBox(-1.0F, 0.0F, 1.0F, 2, 6, 2);
		this.field_48223_b.setRotationPoint(-1.1F, 18.0F, 5.0F);
		this.field_48224_c = new ModelRenderer(this, 40, 0);
		this.field_48224_c.addBox(-1.0F, 0.0F, 0.0F, 2, 10, 2);
		this.field_48224_c.setRotationPoint(1.2F, 13.8F, -5.0F);
		this.field_48221_d = new ModelRenderer(this, 40, 0);
		this.field_48221_d.addBox(-1.0F, 0.0F, 0.0F, 2, 10, 2);
		this.field_48221_d.setRotationPoint(-1.2F, 13.8F, -5.0F);
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		this.setRotationAngles(var2, var3, var4, var5, var6, var7);
		if(this.isChild) {
			float var8 = 2.0F;
			GL11.glPushMatrix();
			GL11.glScalef(1.5F / var8, 1.5F / var8, 1.5F / var8);
			GL11.glTranslatef(0.0F, 10.0F * var7, 4.0F * var7);
			this.field_48220_g.render(var7);
			GL11.glPopMatrix();
			GL11.glPushMatrix();
			GL11.glScalef(1.0F / var8, 1.0F / var8, 1.0F / var8);
			GL11.glTranslatef(0.0F, 24.0F * var7, 0.0F);
			this.field_48226_n.render(var7);
			this.field_48225_a.render(var7);
			this.field_48223_b.render(var7);
			this.field_48224_c.render(var7);
			this.field_48221_d.render(var7);
			this.field_48222_e.render(var7);
			this.field_48219_f.render(var7);
			GL11.glPopMatrix();
		} else {
			this.field_48220_g.render(var7);
			this.field_48226_n.render(var7);
			this.field_48222_e.render(var7);
			this.field_48219_f.render(var7);
			this.field_48225_a.render(var7);
			this.field_48223_b.render(var7);
			this.field_48224_c.render(var7);
			this.field_48221_d.render(var7);
		}

	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		this.field_48220_g.rotateAngleX = var5 / (180.0F / (float)Math.PI);
		this.field_48220_g.rotateAngleY = var4 / (180.0F / (float)Math.PI);
		if(this.field_48227_o != 3) {
			this.field_48226_n.rotateAngleX = (float)Math.PI * 0.5F;
			if(this.field_48227_o == 2) {
				this.field_48225_a.rotateAngleX = MathHelper.cos(var1 * 0.6662F) * 1.0F * var2;
				this.field_48223_b.rotateAngleX = MathHelper.cos(var1 * 0.6662F + 0.3F) * 1.0F * var2;
				this.field_48224_c.rotateAngleX = MathHelper.cos(var1 * 0.6662F + (float)Math.PI + 0.3F) * 1.0F * var2;
				this.field_48221_d.rotateAngleX = MathHelper.cos(var1 * 0.6662F + (float)Math.PI) * 1.0F * var2;
				this.field_48219_f.rotateAngleX = (float)Math.PI * 0.55F + (float)Math.PI * 0.1F * MathHelper.cos(var1) * var2;
			} else {
				this.field_48225_a.rotateAngleX = MathHelper.cos(var1 * 0.6662F) * 1.0F * var2;
				this.field_48223_b.rotateAngleX = MathHelper.cos(var1 * 0.6662F + (float)Math.PI) * 1.0F * var2;
				this.field_48224_c.rotateAngleX = MathHelper.cos(var1 * 0.6662F + (float)Math.PI) * 1.0F * var2;
				this.field_48221_d.rotateAngleX = MathHelper.cos(var1 * 0.6662F) * 1.0F * var2;
				if(this.field_48227_o == 1) {
					this.field_48219_f.rotateAngleX = (float)Math.PI * 0.55F + (float)Math.PI * 0.25F * MathHelper.cos(var1) * var2;
				} else {
					this.field_48219_f.rotateAngleX = (float)Math.PI * 0.55F + (float)Math.PI * 0.15F * MathHelper.cos(var1) * var2;
				}
			}
		}

	}

	public void setLivingAnimations(EntityLiving var1, float var2, float var3, float var4) {
		EntityOcelot var5 = (EntityOcelot)var1;
		this.field_48226_n.rotationPointY = 12.0F;
		this.field_48226_n.rotationPointZ = -10.0F;
		this.field_48220_g.rotationPointY = 15.0F;
		this.field_48220_g.rotationPointZ = -9.0F;
		this.field_48222_e.rotationPointY = 15.0F;
		this.field_48222_e.rotationPointZ = 8.0F;
		this.field_48219_f.rotationPointY = 20.0F;
		this.field_48219_f.rotationPointZ = 14.0F;
		this.field_48224_c.rotationPointY = this.field_48221_d.rotationPointY = 13.8F;
		this.field_48224_c.rotationPointZ = this.field_48221_d.rotationPointZ = -5.0F;
		this.field_48225_a.rotationPointY = this.field_48223_b.rotationPointY = 18.0F;
		this.field_48225_a.rotationPointZ = this.field_48223_b.rotationPointZ = 5.0F;
		this.field_48222_e.rotateAngleX = 0.9F;
		if(var5.isSneaking()) {
			++this.field_48226_n.rotationPointY;
			this.field_48220_g.rotationPointY += 2.0F;
			++this.field_48222_e.rotationPointY;
			this.field_48219_f.rotationPointY += -4.0F;
			this.field_48219_f.rotationPointZ += 2.0F;
			this.field_48222_e.rotateAngleX = (float)Math.PI * 0.5F;
			this.field_48219_f.rotateAngleX = (float)Math.PI * 0.5F;
			this.field_48227_o = 0;
		} else if(var5.isSprinting()) {
			this.field_48219_f.rotationPointY = this.field_48222_e.rotationPointY;
			this.field_48219_f.rotationPointZ += 2.0F;
			this.field_48222_e.rotateAngleX = (float)Math.PI * 0.5F;
			this.field_48219_f.rotateAngleX = (float)Math.PI * 0.5F;
			this.field_48227_o = 2;
		} else if(var5.isSitting()) {
			this.field_48226_n.rotateAngleX = (float)Math.PI * 0.25F;
			this.field_48226_n.rotationPointY += -4.0F;
			this.field_48226_n.rotationPointZ += 5.0F;
			this.field_48220_g.rotationPointY += -3.3F;
			++this.field_48220_g.rotationPointZ;
			this.field_48222_e.rotationPointY += 8.0F;
			this.field_48222_e.rotationPointZ += -2.0F;
			this.field_48219_f.rotationPointY += 2.0F;
			this.field_48219_f.rotationPointZ += -0.8F;
			this.field_48222_e.rotateAngleX = (float)Math.PI * 0.55F;
			this.field_48219_f.rotateAngleX = (float)Math.PI * 0.85F;
			this.field_48224_c.rotateAngleX = this.field_48221_d.rotateAngleX = (float)Math.PI * -0.05F;
			this.field_48224_c.rotationPointY = this.field_48221_d.rotationPointY = 15.8F;
			this.field_48224_c.rotationPointZ = this.field_48221_d.rotationPointZ = -7.0F;
			this.field_48225_a.rotateAngleX = this.field_48223_b.rotateAngleX = (float)Math.PI * -0.5F;
			this.field_48225_a.rotationPointY = this.field_48223_b.rotationPointY = 21.0F;
			this.field_48225_a.rotationPointZ = this.field_48223_b.rotationPointZ = 1.0F;
			this.field_48227_o = 3;
		} else {
			this.field_48227_o = 1;
		}

	}
}
