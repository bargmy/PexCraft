package net.minecraft.src;

import java.util.Random;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

public class RenderItem extends Render {
	private RenderBlocks renderBlocks = new RenderBlocks();
	private Random random = new Random();
	public boolean field_27004_a = true;
	public float zLevel = 0.0F;

	public RenderItem() {
		this.shadowSize = 0.15F;
		this.shadowOpaque = 12.0F / 16.0F;
	}

	public void doRenderItem(EntityItem var1, double var2, double var4, double var6, float var8, float var9) {
		this.random.setSeed(187L);
		ItemStack var10 = var1.item;
		GL11.glPushMatrix();
		float var11 = MathHelper.sin(((float)var1.age + var9) / 10.0F + var1.field_804_d) * 0.1F + 0.1F;
		float var12 = (((float)var1.age + var9) / 20.0F + var1.field_804_d) * (180.0F / (float)Math.PI);
		byte var13 = 1;
		if(var1.item.stackSize > 1) {
			var13 = 2;
		}

		if(var1.item.stackSize > 5) {
			var13 = 3;
		}

		if(var1.item.stackSize > 20) {
			var13 = 4;
		}

		GL11.glTranslatef((float)var2, (float)var4 + var11, (float)var6);
		GL11.glEnable(GL12.GL_RESCALE_NORMAL);
		int var15;
		float var18;
		float var19;
		float var23;
		if(var10.itemID < 256 && RenderBlocks.renderItemIn3d(Block.blocksList[var10.itemID].getRenderType())) {
			GL11.glRotatef(var12, 0.0F, 1.0F, 0.0F);
			this.loadTexture("/terrain.png");
			float var21 = 0.25F;
			var15 = Block.blocksList[var10.itemID].getRenderType();
			if(var15 == 1 || var15 == 19 || var15 == 12 || var15 == 2) {
				var21 = 0.5F;
			}

			GL11.glScalef(var21, var21, var21);

			for(int var22 = 0; var22 < var13; ++var22) {
				GL11.glPushMatrix();
				if(var22 > 0) {
					var23 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.2F / var21;
					var18 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.2F / var21;
					var19 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.2F / var21;
					GL11.glTranslatef(var23, var18, var19);
				}

				var23 = 1.0F;
				this.renderBlocks.renderBlockAsItem(Block.blocksList[var10.itemID], var10.getItemDamage(), var23);
				GL11.glPopMatrix();
			}
		} else {
			int var14;
			float var16;
			if(var10.getItem().func_46058_c()) {
				GL11.glScalef(0.5F, 0.5F, 0.5F);
				this.loadTexture("/gui/items.png");

				for(var14 = 0; var14 <= 1; ++var14) {
					var15 = var10.getItem().func_46057_a(var10.getItemDamage(), var14);
					var16 = 1.0F;
					if(this.field_27004_a) {
						int var17 = Item.itemsList[var10.itemID].getColorFromDamage(var10.getItemDamage(), var14);
						var18 = (float)(var17 >> 16 & 255) / 255.0F;
						var19 = (float)(var17 >> 8 & 255) / 255.0F;
						float var20 = (float)(var17 & 255) / 255.0F;
						GL11.glColor4f(var18 * var16, var19 * var16, var20 * var16, 1.0F);
					}

					this.func_40267_a(var15, var13);
				}
			} else {
				GL11.glScalef(0.5F, 0.5F, 0.5F);
				var14 = var10.getIconIndex();
				if(var10.itemID < 256) {
					this.loadTexture("/terrain.png");
				} else {
					this.loadTexture("/gui/items.png");
				}

				if(this.field_27004_a) {
					var15 = Item.itemsList[var10.itemID].getColorFromDamage(var10.getItemDamage(), 0);
					var16 = (float)(var15 >> 16 & 255) / 255.0F;
					var23 = (float)(var15 >> 8 & 255) / 255.0F;
					var18 = (float)(var15 & 255) / 255.0F;
					var19 = 1.0F;
					GL11.glColor4f(var16 * var19, var23 * var19, var18 * var19, 1.0F);
				}

				this.func_40267_a(var14, var13);
			}
		}

		GL11.glDisable(GL12.GL_RESCALE_NORMAL);
		GL11.glPopMatrix();
	}

	private void func_40267_a(int var1, int var2) {
		Tessellator var3 = Tessellator.instance;
		float var4 = (float)(var1 % 16 * 16 + 0) / 256.0F;
		float var5 = (float)(var1 % 16 * 16 + 16) / 256.0F;
		float var6 = (float)(var1 / 16 * 16 + 0) / 256.0F;
		float var7 = (float)(var1 / 16 * 16 + 16) / 256.0F;
		float var8 = 1.0F;
		float var9 = 0.5F;
		float var10 = 0.25F;

		for(int var11 = 0; var11 < var2; ++var11) {
			GL11.glPushMatrix();
			if(var11 > 0) {
				float var12 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.3F;
				float var13 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.3F;
				float var14 = (this.random.nextFloat() * 2.0F - 1.0F) * 0.3F;
				GL11.glTranslatef(var12, var13, var14);
			}

			GL11.glRotatef(180.0F - this.renderManager.playerViewY, 0.0F, 1.0F, 0.0F);
			var3.startDrawingQuads();
			var3.setNormal(0.0F, 1.0F, 0.0F);
			var3.addVertexWithUV((double)(0.0F - var9), (double)(0.0F - var10), 0.0D, (double)var4, (double)var7);
			var3.addVertexWithUV((double)(var8 - var9), (double)(0.0F - var10), 0.0D, (double)var5, (double)var7);
			var3.addVertexWithUV((double)(var8 - var9), (double)(1.0F - var10), 0.0D, (double)var5, (double)var6);
			var3.addVertexWithUV((double)(0.0F - var9), (double)(1.0F - var10), 0.0D, (double)var4, (double)var6);
			var3.draw();
			GL11.glPopMatrix();
		}

	}

	public void drawItemIntoGui(FontRenderer var1, RenderEngine var2, int var3, int var4, int var5, int var6, int var7) {
		int var10;
		float var11;
		float var12;
		float var13;
		if(var3 < 256 && RenderBlocks.renderItemIn3d(Block.blocksList[var3].getRenderType())) {
			var2.bindTexture(var2.getTexture("/terrain.png"));
			Block var15 = Block.blocksList[var3];
			GL11.glPushMatrix();
			GL11.glTranslatef((float)(var6 - 2), (float)(var7 + 3), -3.0F + this.zLevel);
			GL11.glScalef(10.0F, 10.0F, 10.0F);
			GL11.glTranslatef(1.0F, 0.5F, 1.0F);
			GL11.glScalef(1.0F, 1.0F, -1.0F);
			GL11.glRotatef(210.0F, 1.0F, 0.0F, 0.0F);
			GL11.glRotatef(45.0F, 0.0F, 1.0F, 0.0F);
			var10 = Item.itemsList[var3].getColorFromDamage(var4, 0);
			var11 = (float)(var10 >> 16 & 255) / 255.0F;
			var12 = (float)(var10 >> 8 & 255) / 255.0F;
			var13 = (float)(var10 & 255) / 255.0F;
			if(this.field_27004_a) {
				GL11.glColor4f(var11, var12, var13, 1.0F);
			}

			GL11.glRotatef(-90.0F, 0.0F, 1.0F, 0.0F);
			this.renderBlocks.useInventoryTint = this.field_27004_a;
			this.renderBlocks.renderBlockAsItem(var15, var4, 1.0F);
			this.renderBlocks.useInventoryTint = true;
			GL11.glPopMatrix();
		} else {
			int var8;
			if(Item.itemsList[var3].func_46058_c()) {
				GL11.glDisable(GL11.GL_LIGHTING);
				var2.bindTexture(var2.getTexture("/gui/items.png"));

				for(var8 = 0; var8 <= 1; ++var8) {
					int var9 = Item.itemsList[var3].func_46057_a(var4, var8);
					var10 = Item.itemsList[var3].getColorFromDamage(var4, var8);
					var11 = (float)(var10 >> 16 & 255) / 255.0F;
					var12 = (float)(var10 >> 8 & 255) / 255.0F;
					var13 = (float)(var10 & 255) / 255.0F;
					if(this.field_27004_a) {
						GL11.glColor4f(var11, var12, var13, 1.0F);
					}

					this.renderTexturedQuad(var6, var7, var9 % 16 * 16, var9 / 16 * 16, 16, 16);
				}

				GL11.glEnable(GL11.GL_LIGHTING);
			} else if(var5 >= 0) {
				GL11.glDisable(GL11.GL_LIGHTING);
				if(var3 < 256) {
					var2.bindTexture(var2.getTexture("/terrain.png"));
				} else {
					var2.bindTexture(var2.getTexture("/gui/items.png"));
				}

				var8 = Item.itemsList[var3].getColorFromDamage(var4, 0);
				float var14 = (float)(var8 >> 16 & 255) / 255.0F;
				float var16 = (float)(var8 >> 8 & 255) / 255.0F;
				var11 = (float)(var8 & 255) / 255.0F;
				if(this.field_27004_a) {
					GL11.glColor4f(var14, var16, var11, 1.0F);
				}

				this.renderTexturedQuad(var6, var7, var5 % 16 * 16, var5 / 16 * 16, 16, 16);
				GL11.glEnable(GL11.GL_LIGHTING);
			}
		}

		GL11.glEnable(GL11.GL_CULL_FACE);
	}

	public void renderItemIntoGUI(FontRenderer var1, RenderEngine var2, ItemStack var3, int var4, int var5) {
		if(var3 != null) {
			this.drawItemIntoGui(var1, var2, var3.itemID, var3.getItemDamage(), var3.getIconIndex(), var4, var5);
			if(var3 != null && var3.hasEffect()) {
				GL11.glDepthFunc(GL11.GL_GREATER);
				GL11.glDisable(GL11.GL_LIGHTING);
				GL11.glDepthMask(false);
				var2.bindTexture(var2.getTexture("%blur%/misc/glint.png"));
				this.zLevel -= 50.0F;
				GL11.glEnable(GL11.GL_BLEND);
				GL11.glBlendFunc(GL11.GL_DST_COLOR, GL11.GL_DST_COLOR);
				GL11.glColor4f(0.5F, 0.25F, 0.8F, 1.0F);
				this.func_40266_a(var4 * 431278612 + var5 * 32178161, var4 - 2, var5 - 2, 20, 20);
				GL11.glDisable(GL11.GL_BLEND);
				GL11.glDepthMask(true);
				this.zLevel += 50.0F;
				GL11.glEnable(GL11.GL_LIGHTING);
				GL11.glDepthFunc(GL11.GL_LEQUAL);
			}

		}
	}

	private void func_40266_a(int var1, int var2, int var3, int var4, int var5) {
		for(int var6 = 0; var6 < 2; ++var6) {
			if(var6 == 0) {
				GL11.glBlendFunc(GL11.GL_SRC_COLOR, GL11.GL_ONE);
			}

			if(var6 == 1) {
				GL11.glBlendFunc(GL11.GL_SRC_COLOR, GL11.GL_ONE);
			}

			float var7 = 0.00390625F;
			float var8 = 0.00390625F;
			float var9 = (float)(System.currentTimeMillis() % (long)(3000 + var6 * 1873)) / (3000.0F + (float)(var6 * 1873)) * 256.0F;
			float var10 = 0.0F;
			Tessellator var11 = Tessellator.instance;
			float var12 = 4.0F;
			if(var6 == 1) {
				var12 = -1.0F;
			}

			var11.startDrawingQuads();
			var11.addVertexWithUV((double)(var2 + 0), (double)(var3 + var5), (double)this.zLevel, (double)((var9 + (float)var5 * var12) * var7), (double)((var10 + (float)var5) * var8));
			var11.addVertexWithUV((double)(var2 + var4), (double)(var3 + var5), (double)this.zLevel, (double)((var9 + (float)var4 + (float)var5 * var12) * var7), (double)((var10 + (float)var5) * var8));
			var11.addVertexWithUV((double)(var2 + var4), (double)(var3 + 0), (double)this.zLevel, (double)((var9 + (float)var4) * var7), (double)((var10 + 0.0F) * var8));
			var11.addVertexWithUV((double)(var2 + 0), (double)(var3 + 0), (double)this.zLevel, (double)((var9 + 0.0F) * var7), (double)((var10 + 0.0F) * var8));
			var11.draw();
		}

	}

	public void renderItemOverlayIntoGUI(FontRenderer var1, RenderEngine var2, ItemStack var3, int var4, int var5) {
		if(var3 != null) {
			if(var3.stackSize > 1) {
				String var6 = "" + var3.stackSize;
				GL11.glDisable(GL11.GL_LIGHTING);
				GL11.glDisable(GL11.GL_DEPTH_TEST);
				var1.drawStringWithShadow(var6, var4 + 19 - 2 - var1.getStringWidth(var6), var5 + 6 + 3, 16777215);
				GL11.glEnable(GL11.GL_LIGHTING);
				GL11.glEnable(GL11.GL_DEPTH_TEST);
			}

			if(var3.isItemDamaged()) {
				int var11 = (int)Math.round(13.0D - (double)var3.getItemDamageForDisplay() * 13.0D / (double)var3.getMaxDamage());
				int var7 = (int)Math.round(255.0D - (double)var3.getItemDamageForDisplay() * 255.0D / (double)var3.getMaxDamage());
				GL11.glDisable(GL11.GL_LIGHTING);
				GL11.glDisable(GL11.GL_DEPTH_TEST);
				GL11.glDisable(GL11.GL_TEXTURE_2D);
				Tessellator var8 = Tessellator.instance;
				int var9 = 255 - var7 << 16 | var7 << 8;
				int var10 = (255 - var7) / 4 << 16 | 16128;
				this.renderQuad(var8, var4 + 2, var5 + 13, 13, 2, 0);
				this.renderQuad(var8, var4 + 2, var5 + 13, 12, 1, var10);
				this.renderQuad(var8, var4 + 2, var5 + 13, var11, 1, var9);
				GL11.glEnable(GL11.GL_TEXTURE_2D);
				GL11.glEnable(GL11.GL_LIGHTING);
				GL11.glEnable(GL11.GL_DEPTH_TEST);
				GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
			}

		}
	}

	private void renderQuad(Tessellator var1, int var2, int var3, int var4, int var5, int var6) {
		var1.startDrawingQuads();
		var1.setColorOpaque_I(var6);
		var1.addVertex((double)(var2 + 0), (double)(var3 + 0), 0.0D);
		var1.addVertex((double)(var2 + 0), (double)(var3 + var5), 0.0D);
		var1.addVertex((double)(var2 + var4), (double)(var3 + var5), 0.0D);
		var1.addVertex((double)(var2 + var4), (double)(var3 + 0), 0.0D);
		var1.draw();
	}

	public void renderTexturedQuad(int var1, int var2, int var3, int var4, int var5, int var6) {
		float var7 = 0.00390625F;
		float var8 = 0.00390625F;
		Tessellator var9 = Tessellator.instance;
		var9.startDrawingQuads();
		var9.addVertexWithUV((double)(var1 + 0), (double)(var2 + var6), (double)this.zLevel, (double)((float)(var3 + 0) * var7), (double)((float)(var4 + var6) * var8));
		var9.addVertexWithUV((double)(var1 + var5), (double)(var2 + var6), (double)this.zLevel, (double)((float)(var3 + var5) * var7), (double)((float)(var4 + var6) * var8));
		var9.addVertexWithUV((double)(var1 + var5), (double)(var2 + 0), (double)this.zLevel, (double)((float)(var3 + var5) * var7), (double)((float)(var4 + 0) * var8));
		var9.addVertexWithUV((double)(var1 + 0), (double)(var2 + 0), (double)this.zLevel, (double)((float)(var3 + 0) * var7), (double)((float)(var4 + 0) * var8));
		var9.draw();
	}

	public void doRender(Entity var1, double var2, double var4, double var6, float var8, float var9) {
		this.doRenderItem((EntityItem)var1, var2, var4, var6, var8, var9);
	}
}
