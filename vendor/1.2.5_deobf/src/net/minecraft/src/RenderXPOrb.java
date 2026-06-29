package net.minecraft.src;

import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

public class RenderXPOrb extends Render {
	private RenderBlocks field_35439_b = new RenderBlocks();
	public boolean field_35440_a = true;

	public RenderXPOrb() {
		this.shadowSize = 0.15F;
		this.shadowOpaque = 12.0F / 16.0F;
	}

	public void func_35438_a(EntityXPOrb var1, double var2, double var4, double var6, float var8, float var9) {
		GL11.glPushMatrix();
		GL11.glTranslatef((float)var2, (float)var4, (float)var6);
		int var10 = var1.getTextureByXP();
		this.loadTexture("/item/xporb.png");
		Tessellator var11 = Tessellator.instance;
		float var12 = (float)(var10 % 4 * 16 + 0) / 64.0F;
		float var13 = (float)(var10 % 4 * 16 + 16) / 64.0F;
		float var14 = (float)(var10 / 4 * 16 + 0) / 64.0F;
		float var15 = (float)(var10 / 4 * 16 + 16) / 64.0F;
		float var16 = 1.0F;
		float var17 = 0.5F;
		float var18 = 0.25F;
		int var19 = var1.getBrightnessForRender(var9);
		int var20 = var19 % 65536;
		int var21 = var19 / 65536;
		OpenGlHelper.setLightmapTextureCoords(OpenGlHelper.lightmapTexUnit, (float)var20 / 1.0F, (float)var21 / 1.0F);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		float var26 = 255.0F;
		float var27 = ((float)var1.xpColor + var9) / 2.0F;
		var21 = (int)((MathHelper.sin(var27 + 0.0F) + 1.0F) * 0.5F * var26);
		int var22 = (int)var26;
		int var23 = (int)((MathHelper.sin(var27 + (float)Math.PI * 4.0F / 3.0F) + 1.0F) * 0.1F * var26);
		int var24 = var21 << 16 | var22 << 8 | var23;
		GL11.glRotatef(180.0F - this.renderManager.playerViewY, 0.0F, 1.0F, 0.0F);
		GL11.glRotatef(-this.renderManager.playerViewX, 1.0F, 0.0F, 0.0F);
		float var25 = 0.3F;
		GL11.glScalef(var25, var25, var25);
		var11.startDrawingQuads();
		var11.setColorRGBA_I(var24, 128);
		var11.setNormal(0.0F, 1.0F, 0.0F);
		var11.addVertexWithUV((double)(0.0F - var17), (double)(0.0F - var18), 0.0D, (double)var12, (double)var15);
		var11.addVertexWithUV((double)(var16 - var17), (double)(0.0F - var18), 0.0D, (double)var13, (double)var15);
		var11.addVertexWithUV((double)(var16 - var17), (double)(1.0F - var18), 0.0D, (double)var13, (double)var14);
		var11.addVertexWithUV((double)(0.0F - var17), (double)(1.0F - var18), 0.0D, (double)var12, (double)var14);
		var11.draw();
		GL11.glDisable(GL11.GL_BLEND);
		GL11.glDisable(GL12.GL_RESCALE_NORMAL);
		GL11.glPopMatrix();
	}

	public void doRender(Entity var1, double var2, double var4, double var6, float var8, float var9) {
		this.func_35438_a((EntityXPOrb)var1, var2, var4, var6, var8, var9);
	}
}
