package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class RenderMooshroom extends RenderLiving {
	public RenderMooshroom(ModelBase var1, float var2) {
		super(var1, var2);
	}

	public void func_40273_a(EntityMooshroom var1, double var2, double var4, double var6, float var8, float var9) {
		super.doRenderLiving(var1, var2, var4, var6, var8, var9);
	}

	protected void func_40272_a(EntityMooshroom var1, float var2) {
		super.renderEquippedItems(var1, var2);
		if(!var1.isChild()) {
			this.loadTexture("/terrain.png");
			GL11.glEnable(GL11.GL_CULL_FACE);
			GL11.glPushMatrix();
			GL11.glScalef(1.0F, -1.0F, 1.0F);
			GL11.glTranslatef(0.2F, 0.4F, 0.5F);
			GL11.glRotatef(42.0F, 0.0F, 1.0F, 0.0F);
			this.renderBlocks.renderBlockAsItem(Block.mushroomRed, 0, 1.0F);
			GL11.glTranslatef(0.1F, 0.0F, -0.6F);
			GL11.glRotatef(42.0F, 0.0F, 1.0F, 0.0F);
			this.renderBlocks.renderBlockAsItem(Block.mushroomRed, 0, 1.0F);
			GL11.glPopMatrix();
			GL11.glPushMatrix();
			((ModelQuadruped)this.mainModel).head.postRender(1.0F / 16.0F);
			GL11.glScalef(1.0F, -1.0F, 1.0F);
			GL11.glTranslatef(0.0F, 12.0F / 16.0F, -0.2F);
			GL11.glRotatef(12.0F, 0.0F, 1.0F, 0.0F);
			this.renderBlocks.renderBlockAsItem(Block.mushroomRed, 0, 1.0F);
			GL11.glPopMatrix();
			GL11.glDisable(GL11.GL_CULL_FACE);
		}
	}

	protected void renderEquippedItems(EntityLiving var1, float var2) {
		this.func_40272_a((EntityMooshroom)var1, var2);
	}

	public void doRenderLiving(EntityLiving var1, double var2, double var4, double var6, float var8, float var9) {
		this.func_40273_a((EntityMooshroom)var1, var2, var4, var6, var8, var9);
	}

	public void doRender(Entity var1, double var2, double var4, double var6, float var8, float var9) {
		this.func_40273_a((EntityMooshroom)var1, var2, var4, var6, var8, var9);
	}
}
