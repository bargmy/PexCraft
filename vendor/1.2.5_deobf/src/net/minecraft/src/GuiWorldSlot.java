package net.minecraft.src;

import java.util.Date;

class GuiWorldSlot extends GuiSlot {
	final GuiSelectWorld parentWorldGui;

	public GuiWorldSlot(GuiSelectWorld var1) {
		super(var1.mc, var1.width, var1.height, 32, var1.height - 64, 36);
		this.parentWorldGui = var1;
	}

	protected int getSize() {
		return GuiSelectWorld.getSize(this.parentWorldGui).size();
	}

	protected void elementClicked(int var1, boolean var2) {
		GuiSelectWorld.onElementSelected(this.parentWorldGui, var1);
		boolean var3 = GuiSelectWorld.getSelectedWorld(this.parentWorldGui) >= 0 && GuiSelectWorld.getSelectedWorld(this.parentWorldGui) < this.getSize();
		GuiSelectWorld.getSelectButton(this.parentWorldGui).enabled = var3;
		GuiSelectWorld.getRenameButton(this.parentWorldGui).enabled = var3;
		GuiSelectWorld.getDeleteButton(this.parentWorldGui).enabled = var3;
		if(var2 && var3) {
			this.parentWorldGui.selectWorld(var1);
		}

	}

	protected boolean isSelected(int var1) {
		return var1 == GuiSelectWorld.getSelectedWorld(this.parentWorldGui);
	}

	protected int getContentHeight() {
		return GuiSelectWorld.getSize(this.parentWorldGui).size() * 36;
	}

	protected void drawBackground() {
		this.parentWorldGui.drawDefaultBackground();
	}

	protected void drawSlot(int var1, int var2, int var3, int var4, Tessellator var5) {
		SaveFormatComparator var6 = (SaveFormatComparator)GuiSelectWorld.getSize(this.parentWorldGui).get(var1);
		String var7 = var6.getDisplayName();
		if(var7 == null || MathHelper.stringNullOrLengthZero(var7)) {
			var7 = GuiSelectWorld.getLocalizedWorldName(this.parentWorldGui) + " " + (var1 + 1);
		}

		String var8 = var6.getFileName();
		var8 = var8 + " (" + GuiSelectWorld.getDateFormatter(this.parentWorldGui).format(new Date(var6.getLastTimePlayed()));
		var8 = var8 + ")";
		String var9 = "";
		if(var6.requiresConversion()) {
			var9 = GuiSelectWorld.getLocalizedMustConvert(this.parentWorldGui) + " " + var9;
		} else {
			var9 = GuiSelectWorld.getLocalizedGameMode(this.parentWorldGui)[var6.getGameType()];
			if(var6.isHardcoreModeEnabled()) {
				var9 = "\u00a74" + StatCollector.translateToLocal("gameMode.hardcore") + "\u00a78";
			}
		}

		this.parentWorldGui.drawString(this.parentWorldGui.fontRenderer, var7, var2 + 2, var3 + 1, 16777215);
		this.parentWorldGui.drawString(this.parentWorldGui.fontRenderer, var8, var2 + 2, var3 + 12, 8421504);
		this.parentWorldGui.drawString(this.parentWorldGui.fontRenderer, var9, var2 + 2, var3 + 12 + 10, 8421504);
	}
}
