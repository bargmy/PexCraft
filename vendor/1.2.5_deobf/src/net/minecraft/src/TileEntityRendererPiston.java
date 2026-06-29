package net.minecraft.src;

import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;

public class TileEntityRendererPiston extends TileEntitySpecialRenderer {
	private RenderBlocks blockRenderer;

	public void renderPiston(TileEntityPiston var1, double var2, double var4, double var6, float var8) {
		Block var9 = Block.blocksList[var1.getStoredBlockID()];
		if(var9 != null && var1.getProgress(var8) < 1.0F) {
			Tessellator var10 = Tessellator.instance;
			this.bindTextureByName("/terrain.png");
			RenderHelper.disableStandardItemLighting();
			GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
			GL11.glEnable(GL11.GL_BLEND);
			GL11.glDisable(GL11.GL_CULL_FACE);
			if(Minecraft.isAmbientOcclusionEnabled()) {
				GL11.glShadeModel(GL11.GL_SMOOTH);
			} else {
				GL11.glShadeModel(GL11.GL_FLAT);
			}

			var10.startDrawingQuads();
			var10.setTranslation((double)((float)var2 - (float)var1.xCoord + var1.getOffsetX(var8)), (double)((float)var4 - (float)var1.yCoord + var1.getOffsetY(var8)), (double)((float)var6 - (float)var1.zCoord + var1.getOffsetZ(var8)));
			var10.setColorOpaque(1, 1, 1);
			if(var9 == Block.pistonExtension && var1.getProgress(var8) < 0.5F) {
				this.blockRenderer.renderPistonExtensionAllFaces(var9, var1.xCoord, var1.yCoord, var1.zCoord, false);
			} else if(var1.shouldRenderHead() && !var1.isExtending()) {
				Block.pistonExtension.setHeadTexture(((BlockPistonBase)var9).getPistonExtensionTexture());
				this.blockRenderer.renderPistonExtensionAllFaces(Block.pistonExtension, var1.xCoord, var1.yCoord, var1.zCoord, var1.getProgress(var8) < 0.5F);
				Block.pistonExtension.clearHeadTexture();
				var10.setTranslation((double)((float)var2 - (float)var1.xCoord), (double)((float)var4 - (float)var1.yCoord), (double)((float)var6 - (float)var1.zCoord));
				this.blockRenderer.renderPistonBaseAllFaces(var9, var1.xCoord, var1.yCoord, var1.zCoord);
			} else {
				this.blockRenderer.renderBlockAllFaces(var9, var1.xCoord, var1.yCoord, var1.zCoord);
			}

			var10.setTranslation(0.0D, 0.0D, 0.0D);
			var10.draw();
			RenderHelper.enableStandardItemLighting();
		}

	}

	public void cacheSpecialRenderInfo(World var1) {
		this.blockRenderer = new RenderBlocks(var1);
	}

	public void renderTileEntityAt(TileEntity var1, double var2, double var4, double var6, float var8) {
		this.renderPiston((TileEntityPiston)var1, var2, var4, var6, var8);
	}
}
