package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class ModelEnderCrystal extends ModelBase {
	private ModelRenderer field_41057_g;
	private ModelRenderer field_41058_h = new ModelRenderer(this, "glass");
	private ModelRenderer field_41059_i;

	public ModelEnderCrystal(float var1) {
		this.field_41058_h.setTextureOffset(0, 0).addBox(-4.0F, -4.0F, -4.0F, 8, 8, 8);
		this.field_41057_g = new ModelRenderer(this, "cube");
		this.field_41057_g.setTextureOffset(32, 0).addBox(-4.0F, -4.0F, -4.0F, 8, 8, 8);
		this.field_41059_i = new ModelRenderer(this, "base");
		this.field_41059_i.setTextureOffset(0, 16).addBox(-6.0F, 0.0F, -6.0F, 12, 4, 12);
	}

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
		GL11.glPushMatrix();
		GL11.glScalef(2.0F, 2.0F, 2.0F);
		GL11.glTranslatef(0.0F, -0.5F, 0.0F);
		this.field_41059_i.render(var7);
		GL11.glRotatef(var3, 0.0F, 1.0F, 0.0F);
		GL11.glTranslatef(0.0F, 0.8F + var4, 0.0F);
		GL11.glRotatef(60.0F, 0.7071F, 0.0F, 0.7071F);
		this.field_41058_h.render(var7);
		float var8 = 14.0F / 16.0F;
		GL11.glScalef(var8, var8, var8);
		GL11.glRotatef(60.0F, 0.7071F, 0.0F, 0.7071F);
		GL11.glRotatef(var3, 0.0F, 1.0F, 0.0F);
		this.field_41058_h.render(var7);
		GL11.glScalef(var8, var8, var8);
		GL11.glRotatef(60.0F, 0.7071F, 0.0F, 0.7071F);
		GL11.glRotatef(var3, 0.0F, 1.0F, 0.0F);
		this.field_41057_g.render(var7);
		GL11.glPopMatrix();
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
		super.setRotationAngles(var1, var2, var3, var4, var5, var6);
	}
}
