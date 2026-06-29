package net.minecraft.src;

import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.GL11;

public class GuiContainerCreative extends GuiContainer {
	private static InventoryBasic inventory = new InventoryBasic("tmp", 72);
	private float currentScroll = 0.0F;
	private boolean isScrolling = false;
	private boolean wasClicking;

	public GuiContainerCreative(EntityPlayer var1) {
		super(new ContainerCreative(var1));
		var1.craftingInventory = this.inventorySlots;
		this.allowUserInput = true;
		var1.addStat(AchievementList.openInventory, 1);
		this.ySize = 208;
	}

	public void updateScreen() {
		if(!this.mc.playerController.isInCreativeMode()) {
			this.mc.displayGuiScreen(new GuiInventory(this.mc.thePlayer));
		}

	}

	protected void handleMouseClick(Slot var1, int var2, int var3, boolean var4) {
		InventoryPlayer var5;
		ItemStack var6;
		if(var1 != null) {
			if(var1.inventory == inventory) {
				var5 = this.mc.thePlayer.inventory;
				var6 = var5.getItemStack();
				ItemStack var7 = var1.getStack();
				if(var6 != null && var7 != null && var6.itemID == var7.itemID) {
					if(var3 == 0) {
						if(var4) {
							var6.stackSize = var6.getMaxStackSize();
						} else if(var6.stackSize < var6.getMaxStackSize()) {
							++var6.stackSize;
						}
					} else if(var6.stackSize <= 1) {
						var5.setItemStack((ItemStack)null);
					} else {
						--var6.stackSize;
					}
				} else if(var6 != null) {
					var5.setItemStack((ItemStack)null);
				} else if(var7 == null) {
					var5.setItemStack((ItemStack)null);
				} else if(var6 == null || var6.itemID != var7.itemID) {
					var5.setItemStack(ItemStack.copyItemStack(var7));
					var6 = var5.getItemStack();
					if(var4) {
						var6.stackSize = var6.getMaxStackSize();
					}
				}
			} else {
				this.inventorySlots.slotClick(var1.slotNumber, var3, var4, this.mc.thePlayer);
				ItemStack var8 = this.inventorySlots.getSlot(var1.slotNumber).getStack();
				this.mc.playerController.sendSlotPacket(var8, var1.slotNumber - this.inventorySlots.inventorySlots.size() + 9 + 36);
			}
		} else {
			var5 = this.mc.thePlayer.inventory;
			if(var5.getItemStack() != null) {
				if(var3 == 0) {
					this.mc.thePlayer.dropPlayerItem(var5.getItemStack());
					this.mc.playerController.func_35639_a(var5.getItemStack());
					var5.setItemStack((ItemStack)null);
				}

				if(var3 == 1) {
					var6 = var5.getItemStack().splitStack(1);
					this.mc.thePlayer.dropPlayerItem(var6);
					this.mc.playerController.func_35639_a(var6);
					if(var5.getItemStack().stackSize == 0) {
						var5.setItemStack((ItemStack)null);
					}
				}
			}
		}

	}

	public void initGui() {
		if(!this.mc.playerController.isInCreativeMode()) {
			this.mc.displayGuiScreen(new GuiInventory(this.mc.thePlayer));
		} else {
			super.initGui();
			this.controlList.clear();
		}

	}

	protected void drawGuiContainerForegroundLayer() {
		this.fontRenderer.drawString(StatCollector.translateToLocal("container.creative"), 8, 6, 4210752);
	}

	public void handleMouseInput() {
		super.handleMouseInput();
		int var1 = Mouse.getEventDWheel();
		if(var1 != 0) {
			int var2 = ((ContainerCreative)this.inventorySlots).itemList.size() / 8 - 8 + 1;
			if(var1 > 0) {
				var1 = 1;
			}

			if(var1 < 0) {
				var1 = -1;
			}

			this.currentScroll = (float)((double)this.currentScroll - (double)var1 / (double)var2);
			if(this.currentScroll < 0.0F) {
				this.currentScroll = 0.0F;
			}

			if(this.currentScroll > 1.0F) {
				this.currentScroll = 1.0F;
			}

			((ContainerCreative)this.inventorySlots).scrollTo(this.currentScroll);
		}

	}

	public void drawScreen(int var1, int var2, float var3) {
		boolean var4 = Mouse.isButtonDown(0);
		int var5 = this.guiLeft;
		int var6 = this.guiTop;
		int var7 = var5 + 155;
		int var8 = var6 + 17;
		int var9 = var7 + 14;
		int var10 = var8 + 160 + 2;
		if(!this.wasClicking && var4 && var1 >= var7 && var2 >= var8 && var1 < var9 && var2 < var10) {
			this.isScrolling = true;
		}

		if(!var4) {
			this.isScrolling = false;
		}

		this.wasClicking = var4;
		if(this.isScrolling) {
			this.currentScroll = (float)(var2 - (var8 + 8)) / ((float)(var10 - var8) - 16.0F);
			if(this.currentScroll < 0.0F) {
				this.currentScroll = 0.0F;
			}

			if(this.currentScroll > 1.0F) {
				this.currentScroll = 1.0F;
			}

			((ContainerCreative)this.inventorySlots).scrollTo(this.currentScroll);
		}

		super.drawScreen(var1, var2, var3);
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		GL11.glDisable(GL11.GL_LIGHTING);
	}

	protected void drawGuiContainerBackgroundLayer(float var1, int var2, int var3) {
		GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		int var4 = this.mc.renderEngine.getTexture("/gui/allitems.png");
		this.mc.renderEngine.bindTexture(var4);
		int var5 = this.guiLeft;
		int var6 = this.guiTop;
		this.drawTexturedModalRect(var5, var6, 0, 0, this.xSize, this.ySize);
		int var7 = var5 + 155;
		int var8 = var6 + 17;
		int var9 = var8 + 160 + 2;
		this.drawTexturedModalRect(var5 + 154, var6 + 17 + (int)((float)(var9 - var8 - 17) * this.currentScroll), 0, 208, 16, 16);
	}

	protected void actionPerformed(GuiButton var1) {
		if(var1.id == 0) {
			this.mc.displayGuiScreen(new GuiAchievements(this.mc.statFileWriter));
		}

		if(var1.id == 1) {
			this.mc.displayGuiScreen(new GuiStats(this, this.mc.statFileWriter));
		}

	}

	static InventoryBasic getInventory() {
		return inventory;
	}
}
