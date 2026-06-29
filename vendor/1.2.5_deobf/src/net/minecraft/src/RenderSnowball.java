package net.minecraft.src;

import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

public class RenderSnowball extends Render {
	private int itemIconIndex;

	public RenderSnowball(int var1) {
		this.itemIconIndex = var1;
	}

	public void doRender(Entity var1, double var2, double var4, double var6, float var8, float var9) {
		GL11.glPushMatrix();
		GL11.glTranslatef((float)var2, (float)var4, (float)var6);
		GL11.glEnable(GL12.GL_RESCALE_NORMAL);
		GL11.glScalef(0.5F, 0.5F, 0.5F);
		this.loadTexture("/gui/items.png");
		Tessellator var10 = Tessellator.instance;
		if(this.itemIconIndex == 154) {
			int var11 = PotionHelper.func_40358_a(((EntityPotion)var1).getPotionDamage(), false);
			float var12 = (float)(var11 >> 16 & 255) / 255.0F;
			float var13 = (float)(var11 >> 8 & 255) / 255.0F;
			float var14 = (float)(var11 & 255) / 255.0F;
			GL11.glColor3f(var12, var13, var14);
			GL11.glPushMatrix();
			this.func_40265_a(var10, 141);
			GL11.glPopMatrix();
			GL11.glColor3f(1.0F, 1.0F, 1.0F);
		}

		this.func_40265_a(var10, this.itemIconIndex);
		GL11.glDisable(GL12.GL_RESCALE_NORMAL);
		GL11.glPopMatrix();
	}

	private void func_40265_a(Tessellator var1, int var2) {
		float var3 = (float)(var2 % 16 * 16 + 0) / 256.0F;
		float var4 = (float)(var2 % 16 * 16 + 16) / 256.0F;
		float var5 = (float)(var2 / 16 * 16 + 0) / 256.0F;
		float var6 = (float)(var2 / 16 * 16 + 16) / 256.0F;
		float var7 = 1.0F;
		float var8 = 0.5F;
		float var9 = 0.25F;
		GL11.glRotatef(180.0F - this.renderManager.playerViewY, 0.0F, 1.0F, 0.0F);
		GL11.glRotatef(-this.renderManager.playerViewX, 1.0F, 0.0F, 0.0F);
		var1.startDrawingQuads();
		var1.setNormal(0.0F, 1.0F, 0.0F);
		var1.addVertexWithUV((double)(0.0F - var8), (double)(0.0F - var9), 0.0D, (double)var3, (double)var6);
		var1.addVertexWithUV((double)(var7 - var8), (double)(0.0F - var9), 0.0D, (double)var4, (double)var6);
		var1.addVertexWithUV((double)(var7 - var8), (double)(var7 - var9), 0.0D, (double)var4, (double)var5);
		var1.addVertexWithUV((double)(0.0F - var8), (double)(var7 - var9), 0.0D, (double)var3, (double)var5);
		var1.draw();
	}
}
