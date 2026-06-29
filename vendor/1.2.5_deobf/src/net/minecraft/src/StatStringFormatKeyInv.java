package net.minecraft.src;

import net.minecraft.client.Minecraft;

public class StatStringFormatKeyInv implements IStatStringFormat {
	final Minecraft mc;

	public StatStringFormatKeyInv(Minecraft var1) {
		this.mc = var1;
	}

	public String formatString(String var1) {
		try {
			return String.format(var1, new Object[]{GameSettings.getKeyDisplayString(this.mc.gameSettings.keyBindInventory.keyCode)});
		} catch (Exception var3) {
			return "Error: " + var3.getLocalizedMessage();
		}
	}
}
