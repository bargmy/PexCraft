package net.minecraft.src;

import org.lwjgl.opengl.GL11;

class GuiSlotServer extends GuiSlot {
	final GuiMultiplayer parentGui;

	public GuiSlotServer(GuiMultiplayer var1) {
		super(var1.mc, var1.width, var1.height, 32, var1.height - 64, 36);
		this.parentGui = var1;
	}

	protected int getSize() {
		return GuiMultiplayer.getServerList(this.parentGui).size();
	}

	protected void elementClicked(int var1, boolean var2) {
		GuiMultiplayer.setSelectedServer(this.parentGui, var1);
		boolean var3 = GuiMultiplayer.getSelectedServer(this.parentGui) >= 0 && GuiMultiplayer.getSelectedServer(this.parentGui) < this.getSize();
		GuiMultiplayer.getButtonSelect(this.parentGui).enabled = var3;
		GuiMultiplayer.getButtonEdit(this.parentGui).enabled = var3;
		GuiMultiplayer.getButtonDelete(this.parentGui).enabled = var3;
		if(var2 && var3) {
			GuiMultiplayer.joinServer(this.parentGui, var1);
		}

	}

	protected boolean isSelected(int var1) {
		return var1 == GuiMultiplayer.getSelectedServer(this.parentGui);
	}

	protected int getContentHeight() {
		return GuiMultiplayer.getServerList(this.parentGui).size() * 36;
	}

	protected void drawBackground() {
		this.parentGui.drawDefaultBackground();
	}

	protected void drawSlot(int var1, int var2, int var3, int var4, Tessellator var5) {
		ServerNBTStorage var6 = (ServerNBTStorage)GuiMultiplayer.getServerList(this.parentGui).get(var1);
		Object var7 = GuiMultiplayer.getLock();
		synchronized(var7) {
			if(GuiMultiplayer.getThreadsPending() < 5 && !var6.polled) {
				var6.polled = true;
				var6.lag = -2L;
				var6.motd = "";
				var6.playerCount = "";
				GuiMultiplayer.incrementThreadsPending();
				(new ThreadPollServers(this, var6)).start();
			}
		}

		this.parentGui.drawString(this.parentGui.fontRenderer, var6.name, var2 + 2, var3 + 1, 16777215);
		this.parentGui.drawString(this.parentGui.fontRenderer, var6.motd, var2 + 2, var3 + 12, 8421504);
		this.parentGui.drawString(this.parentGui.fontRenderer, var6.playerCount, var2 + 215 - this.parentGui.fontRenderer.getStringWidth(var6.playerCount), var3 + 12, 8421504);
		this.parentGui.drawString(this.parentGui.fontRenderer, var6.host, var2 + 2, var3 + 12 + 11, 3158064);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		this.parentGui.mc.renderEngine.bindTexture(this.parentGui.mc.renderEngine.getTexture("/gui/icons.png"));
		String var9 = "";
		int var8;
		byte var12;
		if(var6.polled && var6.lag != -2L) {
			var12 = 0;
			boolean var13 = false;
			if(var6.lag < 0L) {
				var8 = 5;
			} else if(var6.lag < 150L) {
				var8 = 0;
			} else if(var6.lag < 300L) {
				var8 = 1;
			} else if(var6.lag < 600L) {
				var8 = 2;
			} else if(var6.lag < 1000L) {
				var8 = 3;
			} else {
				var8 = 4;
			}

			if(var6.lag < 0L) {
				var9 = "(no connection)";
			} else {
				var9 = var6.lag + "ms";
			}
		} else {
			var12 = 1;
			var8 = (int)(System.currentTimeMillis() / 100L + (long)(var1 * 2) & 7L);
			if(var8 > 4) {
				var8 = 8 - var8;
			}

			var9 = "Polling..";
		}

		this.parentGui.drawTexturedModalRect(var2 + 205, var3, 0 + var12 * 10, 176 + var8 * 8, 10, 8);
		byte var10 = 4;
		if(this.mouseX >= var2 + 205 - var10 && this.mouseY >= var3 - var10 && this.mouseX <= var2 + 205 + 10 + var10 && this.mouseY <= var3 + 8 + var10) {
			GuiMultiplayer.setTooltipText(this.parentGui, var9);
		}

	}
}
