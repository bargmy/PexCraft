package net.minecraft.src;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import org.lwjgl.opengl.GL11;

public class GuiWinGame extends GuiScreen {
	private int updateCounter = 0;
	private List lines;
	private int field_41042_d = 0;
	private float field_41043_e = 0.5F;

	public void updateScreen() {
		++this.updateCounter;
		float var1 = (float)(this.field_41042_d + this.height + this.height + 24) / this.field_41043_e;
		if((float)this.updateCounter > var1) {
			this.respawnPlayer();
		}

	}

	protected void keyTyped(char var1, int var2) {
		if(var2 == 1) {
			this.respawnPlayer();
		}

	}

	private void respawnPlayer() {
		if(this.mc.theWorld.isRemote) {
			EntityClientPlayerMP var1 = (EntityClientPlayerMP)this.mc.thePlayer;
			var1.sendQueue.addToSendQueue(new Packet9Respawn(var1.dimension, (byte)this.mc.theWorld.difficultySetting, this.mc.theWorld.getWorldInfo().getTerrainType(), this.mc.theWorld.getHeight(), 0));
		} else {
			this.mc.displayGuiScreen((GuiScreen)null);
			this.mc.respawn(this.mc.theWorld.isRemote, 0, true);
		}

	}

	public boolean doesGuiPauseGame() {
		return true;
	}

	public void initGui() {
		if(this.lines == null) {
			this.lines = new ArrayList();

			try {
				String var1 = "";
				String var2 = "\u00a7f\u00a7k\u00a7a\u00a7b";
				short var3 = 274;
				BufferedReader var4 = new BufferedReader(new InputStreamReader(GuiWinGame.class.getResourceAsStream("/title/win.txt"), Charset.forName("UTF-8")));
				Random var5 = new Random(8124371L);

				while(true) {
					var1 = var4.readLine();
					int var6;
					if(var1 == null) {
						for(var6 = 0; var6 < 8; ++var6) {
							this.lines.add("");
						}

						var4 = new BufferedReader(new InputStreamReader(GuiWinGame.class.getResourceAsStream("/title/credits.txt"), Charset.forName("UTF-8")));

						while(true) {
							var1 = var4.readLine();
							if(var1 == null) {
								this.field_41042_d = this.lines.size() * 12;
								return;
							}

							var1 = var1.replaceAll("PLAYERNAME", this.mc.session.username);
							var1 = var1.replaceAll("\t", "    ");
							this.lines.addAll(this.mc.fontRenderer.func_50108_c(var1, var3));
							this.lines.add("");
						}
					}

					String var7;
					String var8;
					for(var1 = var1.replaceAll("PLAYERNAME", this.mc.session.username); var1.indexOf(var2) >= 0; var1 = var7 + "\u00a7f\u00a7k" + "XXXXXXXX".substring(0, var5.nextInt(4) + 3) + var8) {
						var6 = var1.indexOf(var2);
						var7 = var1.substring(0, var6);
						var8 = var1.substring(var6 + var2.length());
					}

					this.lines.addAll(this.mc.fontRenderer.func_50108_c(var1, var3));
					this.lines.add("");
				}
			} catch (Exception var9) {
				var9.printStackTrace();
			}
		}
	}

	protected void actionPerformed(GuiButton var1) {
	}

	private void func_41040_b(int var1, int var2, float var3) {
		Tessellator var4 = Tessellator.instance;
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("%blur%/gui/background.png"));
		var4.startDrawingQuads();
		var4.setColorRGBA_F(1.0F, 1.0F, 1.0F, 1.0F);
		int var5 = this.width;
		float var6 = 0.0F - ((float)this.updateCounter + var3) * 0.5F * this.field_41043_e;
		float var7 = (float)this.height - ((float)this.updateCounter + var3) * 0.5F * this.field_41043_e;
		float var8 = 0.015625F;
		float var9 = ((float)this.updateCounter + var3 - 0.0F) * 0.02F;
		float var10 = (float)(this.field_41042_d + this.height + this.height + 24) / this.field_41043_e;
		float var11 = (var10 - 20.0F - ((float)this.updateCounter + var3)) * 0.005F;
		if(var11 < var9) {
			var9 = var11;
		}

		if(var9 > 1.0F) {
			var9 = 1.0F;
		}

		var9 *= var9;
		var9 = var9 * 96.0F / 255.0F;
		var4.setColorOpaque_F(var9, var9, var9);
		var4.addVertexWithUV(0.0D, (double)this.height, (double)this.zLevel, 0.0D, (double)(var6 * var8));
		var4.addVertexWithUV((double)var5, (double)this.height, (double)this.zLevel, (double)((float)var5 * var8), (double)(var6 * var8));
		var4.addVertexWithUV((double)var5, 0.0D, (double)this.zLevel, (double)((float)var5 * var8), (double)(var7 * var8));
		var4.addVertexWithUV(0.0D, 0.0D, (double)this.zLevel, 0.0D, (double)(var7 * var8));
		var4.draw();
	}

	public void drawScreen(int var1, int var2, float var3) {
		this.func_41040_b(var1, var2, var3);
		Tessellator var4 = Tessellator.instance;
		short var5 = 274;
		int var6 = this.width / 2 - var5 / 2;
		int var7 = this.height + 50;
		float var8 = -((float)this.updateCounter + var3) * this.field_41043_e;
		GL11.glPushMatrix();
		GL11.glTranslatef(0.0F, var8, 0.0F);
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("/title/mclogo.png"));
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		this.drawTexturedModalRect(var6, var7, 0, 0, 155, 44);
		this.drawTexturedModalRect(var6 + 155, var7, 0, 45, 155, 44);
		var4.setColorOpaque_I(16777215);
		int var9 = var7 + 200;

		int var10;
		for(var10 = 0; var10 < this.lines.size(); ++var10) {
			if(var10 == this.lines.size() - 1) {
				float var11 = (float)var9 + var8 - (float)(this.height / 2 - 6);
				if(var11 < 0.0F) {
					GL11.glTranslatef(0.0F, -var11, 0.0F);
				}
			}

			if((float)var9 + var8 + 12.0F + 8.0F > 0.0F && (float)var9 + var8 < (float)this.height) {
				String var12 = (String)this.lines.get(var10);
				if(var12.startsWith("[C]")) {
					this.fontRenderer.drawStringWithShadow(var12.substring(3), var6 + (var5 - this.fontRenderer.getStringWidth(var12.substring(3))) / 2, var9, 16777215);
				} else {
					this.fontRenderer.fontRandom.setSeed((long)var10 * 4238972211L + (long)(this.updateCounter / 4));
					this.fontRenderer.func_50101_a(var12, var6 + 1, var9 + 1, 16777215, true);
					this.fontRenderer.fontRandom.setSeed((long)var10 * 4238972211L + (long)(this.updateCounter / 4));
					this.fontRenderer.func_50101_a(var12, var6, var9, 16777215, false);
				}
			}

			var9 += 12;
		}

		GL11.glPopMatrix();
		GL11.glBindTexture(GL11.GL_TEXTURE_2D, this.mc.renderEngine.getTexture("%blur%/misc/vignette.png"));
		GL11.glEnable(GL11.GL_BLEND);
		GL11.glBlendFunc(GL11.GL_ZERO, GL11.GL_ONE_MINUS_SRC_COLOR);
		var4.startDrawingQuads();
		var4.setColorRGBA_F(1.0F, 1.0F, 1.0F, 1.0F);
		var10 = this.width;
		int var13 = this.height;
		var4.addVertexWithUV(0.0D, (double)var13, (double)this.zLevel, 0.0D, 1.0D);
		var4.addVertexWithUV((double)var10, (double)var13, (double)this.zLevel, 1.0D, 1.0D);
		var4.addVertexWithUV((double)var10, 0.0D, (double)this.zLevel, 1.0D, 0.0D);
		var4.addVertexWithUV(0.0D, 0.0D, (double)this.zLevel, 0.0D, 0.0D);
		var4.draw();
		GL11.glDisable(GL11.GL_BLEND);
		super.drawScreen(var1, var2, var3);
	}
}
