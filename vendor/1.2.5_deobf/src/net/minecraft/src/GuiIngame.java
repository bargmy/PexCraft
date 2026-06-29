package net.minecraft.src;

import java.awt.Color;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

public class GuiIngame extends Gui {
	private static RenderItem itemRenderer = new RenderItem();
	private List chatMessageList = new ArrayList();
	private List field_50016_f = new ArrayList();
	private Random rand = new Random();
	private Minecraft mc;
	private int updateCounter = 0;
	private String recordPlaying = "";
	private int recordPlayingUpFor = 0;
	private boolean recordIsPlaying = false;
	private int field_50017_n = 0;
	private boolean field_50018_o = false;
	public float damageGuiPartialTime;
	float prevVignetteBrightness = 1.0F;

	public GuiIngame(Minecraft var1) {
		this.mc = var1;
	}

	public void renderGameOverlay(float var1, boolean var2, int var3, int var4) {
		ScaledResolution var5 = new ScaledResolution(this.mc.gameSettings, this.mc.displayWidth, this.mc.displayHeight);
		int var6 = var5.getScaledWidth();
		int var7 = var5.getScaledHeight();
		FontRenderer var8 = this.mc.fontRenderer;
		this.mc.entityRenderer.setupOverlayRendering();
		GL11.glEnable(GL11.GL_BLEND);
		if(Minecraft.isFancyGraphicsEnabled()) {
			this.renderVignette(this.mc.thePlayer.getBrightness(var1), var6, var7);
		} else {
			GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
		}

		ItemStack var9 = this.mc.thePlayer.inventory.armorItemInSlot(3);
		if(this.mc.gameSettings.thirdPersonView == 0 && var9 != null && var9.itemID == Block.pumpkin.blockID) {
			this.renderPumpkinBlur(var6, var7);
		}

		if(!this.mc.thePlayer.isPotionActive(Potion.confusion)) {
			float var10 = this.mc.thePlayer.prevTimeInPortal + (this.mc.thePlayer.timeInPortal - this.mc.thePlayer.prevTimeInPortal) * var1;
			if(var10 > 0.0F) {
				this.renderPortalOverlay(var10, var6, var7);
			}
		}

		boolean var11;
		int var12;
		int var13;
		int var16;
		int var17;
		int var19;
		int var20;
		int var22;
		int var23;
		int var45;
		if(!this.mc.playerController.func_35643_e()) {
			GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
			GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("/gui/gui.png"));
			InventoryPlayer var31 = this.mc.thePlayer.inventory;
			this.zLevel = -90.0F;
			this.drawTexturedModalRect(var6 / 2 - 91, var7 - 22, 0, 0, 182, 22);
			this.drawTexturedModalRect(var6 / 2 - 91 - 1 + var31.currentItem * 20, var7 - 22 - 1, 0, 22, 24, 22);
			GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("/gui/icons.png"));
			GL11.glEnable(GL11.GL_BLEND);
			GL11.glBlendFunc(GL11.GL_ONE_MINUS_DST_COLOR, GL11.GL_ONE_MINUS_SRC_COLOR);
			this.drawTexturedModalRect(var6 / 2 - 7, var7 / 2 - 7, 0, 0, 16, 16);
			GL11.glDisable(GL11.GL_BLEND);
			var11 = this.mc.thePlayer.heartsLife / 3 % 2 == 1;
			if(this.mc.thePlayer.heartsLife < 10) {
				var11 = false;
			}

			var12 = this.mc.thePlayer.getHealth();
			var13 = this.mc.thePlayer.prevHealth;
			this.rand.setSeed((long)(this.updateCounter * 312871));
			boolean var14 = false;
			FoodStats var15 = this.mc.thePlayer.getFoodStats();
			var16 = var15.getFoodLevel();
			var17 = var15.getPrevFoodLevel();
			this.renderBossHealth();
			int var18;
			if(this.mc.playerController.shouldDrawHUD()) {
				var18 = var6 / 2 - 91;
				var19 = var6 / 2 + 91;
				var20 = this.mc.thePlayer.xpBarCap();
				if(var20 > 0) {
					short var21 = 182;
					var22 = (int)(this.mc.thePlayer.experience * (float)(var21 + 1));
					var23 = var7 - 32 + 3;
					this.drawTexturedModalRect(var18, var23, 0, 64, var21, 5);
					if(var22 > 0) {
						this.drawTexturedModalRect(var18, var23, 0, 69, var22, 5);
					}
				}

				var45 = var7 - 39;
				var22 = var45 - 10;
				var23 = this.mc.thePlayer.getTotalArmorValue();
				int var24 = -1;
				if(this.mc.thePlayer.isPotionActive(Potion.regeneration)) {
					var24 = this.updateCounter % 25;
				}

				int var25;
				int var26;
				int var28;
				int var29;
				for(var25 = 0; var25 < 10; ++var25) {
					if(var23 > 0) {
						var26 = var18 + var25 * 8;
						if(var25 * 2 + 1 < var23) {
							this.drawTexturedModalRect(var26, var22, 34, 9, 9, 9);
						}

						if(var25 * 2 + 1 == var23) {
							this.drawTexturedModalRect(var26, var22, 25, 9, 9, 9);
						}

						if(var25 * 2 + 1 > var23) {
							this.drawTexturedModalRect(var26, var22, 16, 9, 9, 9);
						}
					}

					var26 = 16;
					if(this.mc.thePlayer.isPotionActive(Potion.poison)) {
						var26 += 36;
					}

					byte var27 = 0;
					if(var11) {
						var27 = 1;
					}

					var28 = var18 + var25 * 8;
					var29 = var45;
					if(var12 <= 4) {
						var29 = var45 + this.rand.nextInt(2);
					}

					if(var25 == var24) {
						var29 -= 2;
					}

					byte var30 = 0;
					if(this.mc.theWorld.getWorldInfo().isHardcoreModeEnabled()) {
						var30 = 5;
					}

					this.drawTexturedModalRect(var28, var29, 16 + var27 * 9, 9 * var30, 9, 9);
					if(var11) {
						if(var25 * 2 + 1 < var13) {
							this.drawTexturedModalRect(var28, var29, var26 + 54, 9 * var30, 9, 9);
						}

						if(var25 * 2 + 1 == var13) {
							this.drawTexturedModalRect(var28, var29, var26 + 63, 9 * var30, 9, 9);
						}
					}

					if(var25 * 2 + 1 < var12) {
						this.drawTexturedModalRect(var28, var29, var26 + 36, 9 * var30, 9, 9);
					}

					if(var25 * 2 + 1 == var12) {
						this.drawTexturedModalRect(var28, var29, var26 + 45, 9 * var30, 9, 9);
					}
				}

				int var51;
				for(var25 = 0; var25 < 10; ++var25) {
					var26 = var45;
					var51 = 16;
					byte var52 = 0;
					if(this.mc.thePlayer.isPotionActive(Potion.hunger)) {
						var51 += 36;
						var52 = 13;
					}

					if(this.mc.thePlayer.getFoodStats().getSaturationLevel() <= 0.0F && this.updateCounter % (var16 * 3 + 1) == 0) {
						var26 = var45 + (this.rand.nextInt(3) - 1);
					}

					if(var14) {
						var52 = 1;
					}

					var29 = var19 - var25 * 8 - 9;
					this.drawTexturedModalRect(var29, var26, 16 + var52 * 9, 27, 9, 9);
					if(var14) {
						if(var25 * 2 + 1 < var17) {
							this.drawTexturedModalRect(var29, var26, var51 + 54, 27, 9, 9);
						}

						if(var25 * 2 + 1 == var17) {
							this.drawTexturedModalRect(var29, var26, var51 + 63, 27, 9, 9);
						}
					}

					if(var25 * 2 + 1 < var16) {
						this.drawTexturedModalRect(var29, var26, var51 + 36, 27, 9, 9);
					}

					if(var25 * 2 + 1 == var16) {
						this.drawTexturedModalRect(var29, var26, var51 + 45, 27, 9, 9);
					}
				}

				if(this.mc.thePlayer.isInsideOfMaterial(Material.water)) {
					var25 = this.mc.thePlayer.getAir();
					var26 = (int)Math.ceil((double)(var25 - 2) * 10.0D / 300.0D);
					var51 = (int)Math.ceil((double)var25 * 10.0D / 300.0D) - var26;

					for(var28 = 0; var28 < var26 + var51; ++var28) {
						if(var28 < var26) {
							this.drawTexturedModalRect(var19 - var28 * 8 - 9, var22, 16, 18, 9, 9);
						} else {
							this.drawTexturedModalRect(var19 - var28 * 8 - 9, var22, 25, 18, 9, 9);
						}
					}
				}
			}

			GL11.glDisable(GL11.GL_BLEND);
			GL11.glEnable(GL12.GL_RESCALE_NORMAL);
			RenderHelper.enableGUIStandardItemLighting();

			for(var18 = 0; var18 < 9; ++var18) {
				var19 = var6 / 2 - 90 + var18 * 20 + 2;
				var20 = var7 - 16 - 3;
				this.renderInventorySlot(var18, var19, var20, var1);
			}

			RenderHelper.disableStandardItemLighting();
			GL11.glDisable(GL12.GL_RESCALE_NORMAL);
		}

		float var33;
		if(this.mc.thePlayer.getSleepTimer() > 0) {
			GL11.glDisable(GL11.GL_DEPTH_TEST);
			GL11.glDisable(GL11.GL_ALPHA_TEST);
			int var32 = this.mc.thePlayer.getSleepTimer();
			var33 = (float)var32 / 100.0F;
			if(var33 > 1.0F) {
				var33 = 1.0F - (float)(var32 - 100) / 10.0F;
			}

			var12 = (int)(220.0F * var33) << 24 | 1052704;
			drawRect(0, 0, var6, var7, var12);
			GL11.glEnable(GL11.GL_ALPHA_TEST);
			GL11.glEnable(GL11.GL_DEPTH_TEST);
		}

		int var39;
		int var40;
		if(this.mc.playerController.func_35642_f() && this.mc.thePlayer.experienceLevel > 0) {
			var11 = false;
			var12 = var11 ? 16777215 : 8453920;
			String var34 = "" + this.mc.thePlayer.experienceLevel;
			var39 = (var6 - var8.getStringWidth(var34)) / 2;
			var40 = var7 - 31 - 4;
			var8.drawString(var34, var39 + 1, var40, 0);
			var8.drawString(var34, var39 - 1, var40, 0);
			var8.drawString(var34, var39, var40 + 1, 0);
			var8.drawString(var34, var39, var40 - 1, 0);
			var8.drawString(var34, var39, var40, var12);
		}

		if(this.mc.gameSettings.showDebugInfo) {
			GL11.glPushMatrix();
			if(Minecraft.hasPaidCheckTime > 0L) {
				GL11.glTranslatef(0.0F, 32.0F, 0.0F);
			}

			var8.drawStringWithShadow("Minecraft 1.2.5 (" + this.mc.debug + ")", 2, 2, 16777215);
			var8.drawStringWithShadow(this.mc.debugInfoRenders(), 2, 12, 16777215);
			var8.drawStringWithShadow(this.mc.getEntityDebug(), 2, 22, 16777215);
			var8.drawStringWithShadow(this.mc.debugInfoEntities(), 2, 32, 16777215);
			var8.drawStringWithShadow(this.mc.getWorldProviderName(), 2, 42, 16777215);
			long var35 = Runtime.getRuntime().maxMemory();
			long var36 = Runtime.getRuntime().totalMemory();
			long var41 = Runtime.getRuntime().freeMemory();
			long var42 = var36 - var41;
			String var44 = "Used memory: " + var42 * 100L / var35 + "% (" + var42 / 1024L / 1024L + "MB) of " + var35 / 1024L / 1024L + "MB";
			this.drawString(var8, var44, var6 - var8.getStringWidth(var44) - 2, 2, 14737632);
			var44 = "Allocated memory: " + var36 * 100L / var35 + "% (" + var36 / 1024L / 1024L + "MB)";
			this.drawString(var8, var44, var6 - var8.getStringWidth(var44) - 2, 12, 14737632);
			this.drawString(var8, "x: " + this.mc.thePlayer.posX, 2, 64, 14737632);
			this.drawString(var8, "y: " + this.mc.thePlayer.posY, 2, 72, 14737632);
			this.drawString(var8, "z: " + this.mc.thePlayer.posZ, 2, 80, 14737632);
			this.drawString(var8, "f: " + (MathHelper.floor_double((double)(this.mc.thePlayer.rotationYaw * 4.0F / 360.0F) + 0.5D) & 3), 2, 88, 14737632);
			var45 = MathHelper.floor_double(this.mc.thePlayer.posX);
			var22 = MathHelper.floor_double(this.mc.thePlayer.posY);
			var23 = MathHelper.floor_double(this.mc.thePlayer.posZ);
			if(this.mc.theWorld != null && this.mc.theWorld.blockExists(var45, var22, var23)) {
				Chunk var48 = this.mc.theWorld.getChunkFromBlockCoords(var45, var23);
				this.drawString(var8, "lc: " + (var48.getTopFilledSegment() + 15) + " b: " + var48.func_48490_a(var45 & 15, var23 & 15, this.mc.theWorld.getWorldChunkManager()).biomeName + " bl: " + var48.getSavedLightValue(EnumSkyBlock.Block, var45 & 15, var22, var23 & 15) + " sl: " + var48.getSavedLightValue(EnumSkyBlock.Sky, var45 & 15, var22, var23 & 15) + " rl: " + var48.getBlockLightValue(var45 & 15, var22, var23 & 15, 0), 2, 96, 14737632);
			}

			if(!this.mc.theWorld.isRemote) {
				this.drawString(var8, "Seed: " + this.mc.theWorld.getSeed(), 2, 112, 14737632);
			}

			GL11.glPopMatrix();
		}

		if(this.recordPlayingUpFor > 0) {
			var33 = (float)this.recordPlayingUpFor - var1;
			var12 = (int)(var33 * 256.0F / 20.0F);
			if(var12 > 255) {
				var12 = 255;
			}

			if(var12 > 0) {
				GL11.glPushMatrix();
				GL11.glTranslatef((float)(var6 / 2), (float)(var7 - 48), 0.0F);
				GL11.glEnable(GL11.GL_BLEND);
				GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
				var13 = 16777215;
				if(this.recordIsPlaying) {
					var13 = Color.HSBtoRGB(var33 / 50.0F, 0.7F, 0.6F) & 16777215;
				}

				var8.drawString(this.recordPlaying, -var8.getStringWidth(this.recordPlaying) / 2, -4, var13 + (var12 << 24));
				GL11.glDisable(GL11.GL_BLEND);
				GL11.glPopMatrix();
			}
		}

		GL11.glEnable(GL11.GL_BLEND);
		GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
		GL11.glDisable(GL11.GL_ALPHA_TEST);
		GL11.glPushMatrix();
		GL11.glTranslatef(0.0F, (float)(var7 - 48), 0.0F);
		this.func_50010_a(var8);
		GL11.glPopMatrix();
		if(this.mc.thePlayer instanceof EntityClientPlayerMP && this.mc.gameSettings.keyBindPlayerList.pressed) {
			NetClientHandler var37 = ((EntityClientPlayerMP)this.mc.thePlayer).sendQueue;
			List var38 = var37.playerNames;
			var13 = var37.currentServerMaxPlayers;
			var39 = var13;

			for(var40 = 1; var39 > 20; var39 = (var13 + var40 - 1) / var40) {
				++var40;
			}

			var16 = 300 / var40;
			if(var16 > 150) {
				var16 = 150;
			}

			var17 = (var6 - var40 * var16) / 2;
			byte var43 = 10;
			drawRect(var17 - 1, var43 - 1, var17 + var16 * var40, var43 + 9 * var39, Integer.MIN_VALUE);

			for(var19 = 0; var19 < var13; ++var19) {
				var20 = var17 + var19 % var40 * var16;
				var45 = var43 + var19 / var40 * 9;
				drawRect(var20, var45, var20 + var16 - 1, var45 + 8, 553648127);
				GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
				GL11.glEnable(GL11.GL_ALPHA_TEST);
				if(var19 < var38.size()) {
					GuiPlayerInfo var46 = (GuiPlayerInfo)var38.get(var19);
					var8.drawStringWithShadow(var46.name, var20, var45, 16777215);
					this.mc.renderEngine.bindTexture(this.mc.renderEngine.getTexture("/gui/icons.png"));
					byte var47 = 0;
					boolean var49 = false;
					byte var50;
					if(var46.responseTime < 0) {
						var50 = 5;
					} else if(var46.responseTime < 150) {
						var50 = 0;
					} else if(var46.responseTime < 300) {
						var50 = 1;
					} else if(var46.responseTime < 600) {
						var50 = 2;
					} else if(var46.responseTime < 1000) {
						var50 = 3;
					} else {
						var50 = 4;
					}

					this.zLevel += 100.0F;
					this.drawTexturedModalRect(var20 + var16 - 12, var45, 0 + var47 * 10, 176 + var50 * 8, 10, 8);
					this.zLevel -= 100.0F;
				}
			}
		}

		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		GL11.glDisable(GL11.GL_LIGHTING);
		GL11.glEnable(GL11.GL_ALPHA_TEST);
	}

	private void func_50010_a(FontRenderer var1) {
		byte var2 = 10;
		boolean var3 = false;
		int var4 = 0;
		int var5 = this.chatMessageList.size();
		if(var5 > 0) {
			if(this.isChatOpen()) {
				var2 = 20;
				var3 = true;
			}

			int var6;
			int var10;
			for(var6 = 0; var6 + this.field_50017_n < this.chatMessageList.size() && var6 < var2; ++var6) {
				if(((ChatLine)this.chatMessageList.get(var6)).updateCounter < 200 || var3) {
					ChatLine var7 = (ChatLine)this.chatMessageList.get(var6 + this.field_50017_n);
					double var8 = (double)var7.updateCounter / 200.0D;
					var8 = 1.0D - var8;
					var8 *= 10.0D;
					if(var8 < 0.0D) {
						var8 = 0.0D;
					}

					if(var8 > 1.0D) {
						var8 = 1.0D;
					}

					var8 *= var8;
					var10 = (int)(255.0D * var8);
					if(var3) {
						var10 = 255;
					}

					++var4;
					if(var10 > 2) {
						byte var11 = 3;
						int var12 = -var6 * 9;
						String var13 = var7.message;
						drawRect(var11, var12 - 1, var11 + 320 + 4, var12 + 8, var10 / 2 << 24);
						GL11.glEnable(GL11.GL_BLEND);
						var1.drawStringWithShadow(var13, var11, var12, 16777215 + (var10 << 24));
					}
				}
			}

			if(var3) {
				GL11.glTranslatef(0.0F, (float)var1.FONT_HEIGHT, 0.0F);
				var6 = var5 * var1.FONT_HEIGHT + var5;
				int var14 = var4 * var1.FONT_HEIGHT + var4;
				int var15 = this.field_50017_n * var14 / var5;
				int var9 = var14 * var14 / var6;
				if(var6 != var14) {
					var10 = var15 > 0 ? 170 : 96;
					int var16 = this.field_50018_o ? 13382451 : 3355562;
					drawRect(0, -var15, 2, -var15 - var9, var16 + (var10 << 24));
					drawRect(2, -var15, 1, -var15 - var9, 13421772 + (var10 << 24));
				}
			}

		}
	}

	private void renderBossHealth() {
		if(RenderDragon.entityDragon != null) {
			EntityDragon var1 = RenderDragon.entityDragon;
			RenderDragon.entityDragon = null;
			FontRenderer var2 = this.mc.fontRenderer;
			ScaledResolution var3 = new ScaledResolution(this.mc.gameSettings, this.mc.displayWidth, this.mc.displayHeight);
			int var4 = var3.getScaledWidth();
			short var5 = 182;
			int var6 = var4 / 2 - var5 / 2;
			int var7 = (int)((float)var1.func_41010_ax() / (float)var1.getMaxHealth() * (float)(var5 + 1));
			byte var8 = 12;
			this.drawTexturedModalRect(var6, var8, 0, 74, var5, 5);
			this.drawTexturedModalRect(var6, var8, 0, 74, var5, 5);
			if(var7 > 0) {
				this.drawTexturedModalRect(var6, var8, 0, 79, var7, 5);
			}

			String var9 = "Boss health";
			var2.drawStringWithShadow(var9, var4 / 2 - var2.getStringWidth(var9) / 2, var8 - 10, 16711935);
			GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
			GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("/gui/icons.png"));
		}
	}

	private void renderPumpkinBlur(int var1, int var2) {
		GL11.glDisable(GL11.GL_DEPTH_TEST);
		GL11.glDepthMask(false);
		GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		GL11.glDisable(GL11.GL_ALPHA_TEST);
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("%blur%/misc/pumpkinblur.png"));
		Tessellator var3 = Tessellator.instance;
		var3.startDrawingQuads();
		var3.addVertexWithUV(0.0D, (double)var2, -90.0D, 0.0D, 1.0D);
		var3.addVertexWithUV((double)var1, (double)var2, -90.0D, 1.0D, 1.0D);
		var3.addVertexWithUV((double)var1, 0.0D, -90.0D, 1.0D, 0.0D);
		var3.addVertexWithUV(0.0D, 0.0D, -90.0D, 0.0D, 0.0D);
		var3.draw();
		GL11.glDepthMask(true);
		GL11.glEnable(GL11.GL_DEPTH_TEST);
		GL11.glEnable(GL11.GL_ALPHA_TEST);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
	}

	private void renderVignette(float var1, int var2, int var3) {
		var1 = 1.0F - var1;
		if(var1 < 0.0F) {
			var1 = 0.0F;
		}

		if(var1 > 1.0F) {
			var1 = 1.0F;
		}

		this.prevVignetteBrightness = (float)((double)this.prevVignetteBrightness + (double)(var1 - this.prevVignetteBrightness) * 0.01D);
		GL11.glDisable(GL11.GL_DEPTH_TEST);
		GL11.glDepthMask(false);
		GL11.glBlendFunc(GL11.GL_ZERO, GL11.GL_ONE_MINUS_SRC_COLOR);
		GL11.glColor4f(this.prevVignetteBrightness, this.prevVignetteBrightness, this.prevVignetteBrightness, 1.0F);
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("%blur%/misc/vignette.png"));
		Tessellator var4 = Tessellator.instance;
		var4.startDrawingQuads();
		var4.addVertexWithUV(0.0D, (double)var3, -90.0D, 0.0D, 1.0D);
		var4.addVertexWithUV((double)var2, (double)var3, -90.0D, 1.0D, 1.0D);
		var4.addVertexWithUV((double)var2, 0.0D, -90.0D, 1.0D, 0.0D);
		var4.addVertexWithUV(0.0D, 0.0D, -90.0D, 0.0D, 0.0D);
		var4.draw();
		GL11.glDepthMask(true);
		GL11.glEnable(GL11.GL_DEPTH_TEST);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
	}

	private void renderPortalOverlay(float var1, int var2, int var3) {
		if(var1 < 1.0F) {
			var1 *= var1;
			var1 *= var1;
			var1 = var1 * 0.8F + 0.2F;
		}

		GL11.glDisable(GL11.GL_ALPHA_TEST);
		GL11.glDisable(GL11.GL_DEPTH_TEST);
		GL11.glDepthMask(false);
		GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, var1);
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("/terrain.png"));
		float var4 = (float)(Block.portal.blockIndexInTexture % 16) / 16.0F;
		float var5 = (float)(Block.portal.blockIndexInTexture / 16) / 16.0F;
		float var6 = (float)(Block.portal.blockIndexInTexture % 16 + 1) / 16.0F;
		float var7 = (float)(Block.portal.blockIndexInTexture / 16 + 1) / 16.0F;
		Tessellator var8 = Tessellator.instance;
		var8.startDrawingQuads();
		var8.addVertexWithUV(0.0D, (double)var3, -90.0D, (double)var4, (double)var7);
		var8.addVertexWithUV((double)var2, (double)var3, -90.0D, (double)var6, (double)var7);
		var8.addVertexWithUV((double)var2, 0.0D, -90.0D, (double)var6, (double)var5);
		var8.addVertexWithUV(0.0D, 0.0D, -90.0D, (double)var4, (double)var5);
		var8.draw();
		GL11.glDepthMask(true);
		GL11.glEnable(GL11.GL_DEPTH_TEST);
		GL11.glEnable(GL11.GL_ALPHA_TEST);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
	}

	private void renderInventorySlot(int var1, int var2, int var3, float var4) {
		ItemStack var5 = this.mc.thePlayer.inventory.mainInventory[var1];
		if(var5 != null) {
			float var6 = (float)var5.animationsToGo - var4;
			if(var6 > 0.0F) {
				GL11.glPushMatrix();
				float var7 = 1.0F + var6 / 5.0F;
				GL11.glTranslatef((float)(var2 + 8), (float)(var3 + 12), 0.0F);
				GL11.glScalef(1.0F / var7, (var7 + 1.0F) / 2.0F, 1.0F);
				GL11.glTranslatef((float)(-(var2 + 8)), (float)(-(var3 + 12)), 0.0F);
			}

			itemRenderer.renderItemIntoGUI(this.mc.fontRenderer, this.mc.renderEngine, var5, var2, var3);
			if(var6 > 0.0F) {
				GL11.glPopMatrix();
			}

			itemRenderer.renderItemOverlayIntoGUI(this.mc.fontRenderer, this.mc.renderEngine, var5, var2, var3);
		}
	}

	public void updateTick() {
		if(this.recordPlayingUpFor > 0) {
			--this.recordPlayingUpFor;
		}

		++this.updateCounter;

		for(int var1 = 0; var1 < this.chatMessageList.size(); ++var1) {
			++((ChatLine)this.chatMessageList.get(var1)).updateCounter;
		}

	}

	public void clearChatMessages() {
		this.chatMessageList.clear();
		this.field_50016_f.clear();
	}

	public void addChatMessage(String var1) {
		boolean var2 = this.isChatOpen();
		boolean var3 = true;
		Iterator var4 = this.mc.fontRenderer.func_50108_c(var1, 320).iterator();

		while(var4.hasNext()) {
			String var5 = (String)var4.next();
			if(var2 && this.field_50017_n > 0) {
				this.field_50018_o = true;
				this.func_50011_a(1);
			}

			if(!var3) {
				var5 = " " + var5;
			}

			var3 = false;
			this.chatMessageList.add(0, new ChatLine(var5));
		}

		while(this.chatMessageList.size() > 100) {
			this.chatMessageList.remove(this.chatMessageList.size() - 1);
		}

	}

	public List func_50013_c() {
		return this.field_50016_f;
	}

	public void func_50014_d() {
		this.field_50017_n = 0;
		this.field_50018_o = false;
	}

	public void func_50011_a(int var1) {
		this.field_50017_n += var1;
		int var2 = this.chatMessageList.size();
		if(this.field_50017_n > var2 - 20) {
			this.field_50017_n = var2 - 20;
		}

		if(this.field_50017_n <= 0) {
			this.field_50017_n = 0;
			this.field_50018_o = false;
		}

	}

	public ChatClickData func_50012_a(int var1, int var2) {
		if(!this.isChatOpen()) {
			return null;
		} else {
			ScaledResolution var3 = new ScaledResolution(this.mc.gameSettings, this.mc.displayWidth, this.mc.displayHeight);
			var2 = var2 / var3.scaleFactor - 40;
			var1 = var1 / var3.scaleFactor - 3;
			if(var1 >= 0 && var2 >= 0) {
				int var4 = Math.min(20, this.chatMessageList.size());
				if(var1 <= 320 && var2 < this.mc.fontRenderer.FONT_HEIGHT * var4 + var4) {
					int var5 = var2 / (this.mc.fontRenderer.FONT_HEIGHT + 1) + this.field_50017_n;
					return new ChatClickData(this.mc.fontRenderer, (ChatLine)this.chatMessageList.get(var5), var1, var2 - (var5 - this.field_50017_n) * this.mc.fontRenderer.FONT_HEIGHT + var5);
				} else {
					return null;
				}
			} else {
				return null;
			}
		}
	}

	public void setRecordPlayingMessage(String var1) {
		this.recordPlaying = "Now playing: " + var1;
		this.recordPlayingUpFor = 60;
		this.recordIsPlaying = true;
	}

	public boolean isChatOpen() {
		return this.mc.currentScreen instanceof GuiChat;
	}

	public void addChatMessageTranslate(String var1) {
		StringTranslate var2 = StringTranslate.getInstance();
		String var3 = var2.translateKey(var1);
		this.addChatMessage(var3);
	}
}
