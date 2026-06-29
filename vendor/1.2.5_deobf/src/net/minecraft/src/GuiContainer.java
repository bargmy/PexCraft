package net.minecraft.src;

import java.util.List;
import org.lwjgl.input.Keyboard;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

public abstract class GuiContainer extends GuiScreen {
	protected static RenderItem itemRenderer = new RenderItem();
	protected int xSize = 176;
	protected int ySize = 166;
	public Container inventorySlots;
	protected int guiLeft;
	protected int guiTop;

	public GuiContainer(Container var1) {
		this.inventorySlots = var1;
	}

	public void initGui() {
		super.initGui();
		this.mc.thePlayer.craftingInventory = this.inventorySlots;
		this.guiLeft = (this.width - this.xSize) / 2;
		this.guiTop = (this.height - this.ySize) / 2;
	}

	public void drawScreen(int var1, int var2, float var3) {
		this.drawDefaultBackground();
		int var4 = this.guiLeft;
		int var5 = this.guiTop;
		this.drawGuiContainerBackgroundLayer(var3, var1, var2);
		RenderHelper.enableGUIStandardItemLighting();
		GL11.glPushMatrix();
		GL11.glTranslatef((float)var4, (float)var5, 0.0F);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		GL11.glEnable(GL12.GL_RESCALE_NORMAL);
		Slot var6 = null;
		short var7 = 240;
		short var8 = 240;
		OpenGlHelper.setLightmapTextureCoords(OpenGlHelper.lightmapTexUnit, (float)var7 / 1.0F, (float)var8 / 1.0F);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);

		int var10;
		for(int var20 = 0; var20 < this.inventorySlots.inventorySlots.size(); ++var20) {
			Slot var22 = (Slot)this.inventorySlots.inventorySlots.get(var20);
			this.drawSlotInventory(var22);
			if(this.isMouseOverSlot(var22, var1, var2)) {
				var6 = var22;
				GL11.glDisable(GL11.GL_LIGHTING);
				GL11.glDisable(GL11.GL_DEPTH_TEST);
				int var9 = var22.xDisplayPosition;
				var10 = var22.yDisplayPosition;
				this.drawGradientRect(var9, var10, var9 + 16, var10 + 16, -2130706433, -2130706433);
				GL11.glEnable(GL11.GL_LIGHTING);
				GL11.glEnable(GL11.GL_DEPTH_TEST);
			}
		}

		this.drawGuiContainerForegroundLayer();
		InventoryPlayer var21 = this.mc.thePlayer.inventory;
		if(var21.getItemStack() != null) {
			GL11.glTranslatef(0.0F, 0.0F, 32.0F);
			this.zLevel = 200.0F;
			itemRenderer.zLevel = 200.0F;
			itemRenderer.renderItemIntoGUI(this.fontRenderer, this.mc.renderEngine, var21.getItemStack(), var1 - var4 - 8, var2 - var5 - 8);
			itemRenderer.renderItemOverlayIntoGUI(this.fontRenderer, this.mc.renderEngine, var21.getItemStack(), var1 - var4 - 8, var2 - var5 - 8);
			this.zLevel = 0.0F;
			itemRenderer.zLevel = 0.0F;
		}

		GL11.glDisable(GL12.GL_RESCALE_NORMAL);
		RenderHelper.disableStandardItemLighting();
		GL11.glDisable(GL11.GL_LIGHTING);
		GL11.glDisable(GL11.GL_DEPTH_TEST);
		if(var21.getItemStack() == null && var6 != null && var6.getHasStack()) {
			ItemStack var23 = var6.getStack();
			List var24 = var23.getItemNameandInformation();
			if(var24.size() > 0) {
				var10 = 0;

				int var11;
				int var12;
				for(var11 = 0; var11 < var24.size(); ++var11) {
					var12 = this.fontRenderer.getStringWidth((String)var24.get(var11));
					if(var12 > var10) {
						var10 = var12;
					}
				}

				var11 = var1 - var4 + 12;
				var12 = var2 - var5 - 12;
				int var14 = 8;
				if(var24.size() > 1) {
					var14 += 2 + (var24.size() - 1) * 10;
				}

				this.zLevel = 300.0F;
				itemRenderer.zLevel = 300.0F;
				int var15 = -267386864;
				this.drawGradientRect(var11 - 3, var12 - 4, var11 + var10 + 3, var12 - 3, var15, var15);
				this.drawGradientRect(var11 - 3, var12 + var14 + 3, var11 + var10 + 3, var12 + var14 + 4, var15, var15);
				this.drawGradientRect(var11 - 3, var12 - 3, var11 + var10 + 3, var12 + var14 + 3, var15, var15);
				this.drawGradientRect(var11 - 4, var12 - 3, var11 - 3, var12 + var14 + 3, var15, var15);
				this.drawGradientRect(var11 + var10 + 3, var12 - 3, var11 + var10 + 4, var12 + var14 + 3, var15, var15);
				int var16 = 1347420415;
				int var17 = (var16 & 16711422) >> 1 | var16 & -16777216;
				this.drawGradientRect(var11 - 3, var12 - 3 + 1, var11 - 3 + 1, var12 + var14 + 3 - 1, var16, var17);
				this.drawGradientRect(var11 + var10 + 2, var12 - 3 + 1, var11 + var10 + 3, var12 + var14 + 3 - 1, var16, var17);
				this.drawGradientRect(var11 - 3, var12 - 3, var11 + var10 + 3, var12 - 3 + 1, var16, var16);
				this.drawGradientRect(var11 - 3, var12 + var14 + 2, var11 + var10 + 3, var12 + var14 + 3, var17, var17);

				for(int var18 = 0; var18 < var24.size(); ++var18) {
					String var19 = (String)var24.get(var18);
					if(var18 == 0) {
						var19 = "\u00a7" + Integer.toHexString(var23.getRarity().nameColor) + var19;
					} else {
						var19 = "\u00a77" + var19;
					}

					this.fontRenderer.drawStringWithShadow(var19, var11, var12, -1);
					if(var18 == 0) {
						var12 += 2;
					}

					var12 += 10;
				}

				this.zLevel = 0.0F;
				itemRenderer.zLevel = 0.0F;
			}
		}

		GL11.glPopMatrix();
		super.drawScreen(var1, var2, var3);
		GL11.glEnable(GL11.GL_LIGHTING);
		GL11.glEnable(GL11.GL_DEPTH_TEST);
	}

	protected void drawGuiContainerForegroundLayer() {
	}

	protected abstract void drawGuiContainerBackgroundLayer(float var1, int var2, int var3);

	private void drawSlotInventory(Slot var1) {
		int var2 = var1.xDisplayPosition;
		int var3 = var1.yDisplayPosition;
		ItemStack var4 = var1.getStack();
		boolean var5 = false;
		this.zLevel = 100.0F;
		itemRenderer.zLevel = 100.0F;
		if(var4 == null) {
			int var8 = var1.getBackgroundIconIndex();
			if(var8 >= 0) {
				GL11.glDisable(GL11.GL_LIGHTING);
				this.mc.renderEngine.bindTexture(this.mc.renderEngine.getTexture("/gui/items.png"));
				this.drawTexturedModalRect(var2, var3, var8 % 16 * 16, var8 / 16 * 16, 16, 16);
				GL11.glEnable(GL11.GL_LIGHTING);
				var5 = true;
			}
		}

		if(!var5) {
			itemRenderer.renderItemIntoGUI(this.fontRenderer, this.mc.renderEngine, var4, var2, var3);
			itemRenderer.renderItemOverlayIntoGUI(this.fontRenderer, this.mc.renderEngine, var4, var2, var3);
		}

		itemRenderer.zLevel = 0.0F;
		this.zLevel = 0.0F;
	}

	private Slot getSlotAtPosition(int var1, int var2) {
		for(int var3 = 0; var3 < this.inventorySlots.inventorySlots.size(); ++var3) {
			Slot var4 = (Slot)this.inventorySlots.inventorySlots.get(var3);
			if(this.isMouseOverSlot(var4, var1, var2)) {
				return var4;
			}
		}

		return null;
	}

	protected void mouseClicked(int var1, int var2, int var3) {
		super.mouseClicked(var1, var2, var3);
		if(var3 == 0 || var3 == 1) {
			Slot var4 = this.getSlotAtPosition(var1, var2);
			int var5 = this.guiLeft;
			int var6 = this.guiTop;
			boolean var7 = var1 < var5 || var2 < var6 || var1 >= var5 + this.xSize || var2 >= var6 + this.ySize;
			int var8 = -1;
			if(var4 != null) {
				var8 = var4.slotNumber;
			}

			if(var7) {
				var8 = -999;
			}

			if(var8 != -1) {
				boolean var9 = var8 != -999 && (Keyboard.isKeyDown(Keyboard.KEY_LSHIFT) || Keyboard.isKeyDown(Keyboard.KEY_RSHIFT));
				this.handleMouseClick(var4, var8, var3, var9);
			}
		}

	}

	private boolean isMouseOverSlot(Slot var1, int var2, int var3) {
		int var4 = this.guiLeft;
		int var5 = this.guiTop;
		var2 -= var4;
		var3 -= var5;
		return var2 >= var1.xDisplayPosition - 1 && var2 < var1.xDisplayPosition + 16 + 1 && var3 >= var1.yDisplayPosition - 1 && var3 < var1.yDisplayPosition + 16 + 1;
	}

	protected void handleMouseClick(Slot var1, int var2, int var3, boolean var4) {
		if(var1 != null) {
			var2 = var1.slotNumber;
		}

		this.mc.playerController.windowClick(this.inventorySlots.windowId, var2, var3, var4, this.mc.thePlayer);
	}

	protected void keyTyped(char var1, int var2) {
		if(var2 == 1 || var2 == this.mc.gameSettings.keyBindInventory.keyCode) {
			this.mc.thePlayer.closeScreen();
		}

	}

	public void onGuiClosed() {
		if(this.mc.thePlayer != null) {
			this.inventorySlots.onCraftGuiClosed(this.mc.thePlayer);
			this.mc.playerController.func_20086_a(this.inventorySlots.windowId, this.mc.thePlayer);
		}
	}

	public boolean doesGuiPauseGame() {
		return false;
	}

	public void updateScreen() {
		super.updateScreen();
		if(!this.mc.thePlayer.isEntityAlive() || this.mc.thePlayer.isDead) {
			this.mc.thePlayer.closeScreen();
		}

	}
}
