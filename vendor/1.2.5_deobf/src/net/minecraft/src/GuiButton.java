package net.minecraft.src;

import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;

public class GuiButton extends Gui {
	protected int field_52008_a;
	protected int field_52007_b;
	public int xPosition;
	public int yPosition;
	public String displayString;
	public int id;
	public boolean enabled;
	public boolean drawButton;

	public GuiButton(int var1, int var2, int var3, String var4) {
		this(var1, var2, var3, 200, 20, var4);
	}

	public GuiButton(int var1, int var2, int var3, int var4, int var5, String var6) {
		this.field_52008_a = 200;
		this.field_52007_b = 20;
		this.enabled = true;
		this.drawButton = true;
		this.id = var1;
		this.xPosition = var2;
		this.yPosition = var3;
		this.field_52008_a = var4;
		this.field_52007_b = var5;
		this.displayString = var6;
	}

	protected int getHoverState(boolean var1) {
		byte var2 = 1;
		if(!this.enabled) {
			var2 = 0;
		} else if(var1) {
			var2 = 2;
		}

		return var2;
	}

	public void drawButton(Minecraft var1, int var2, int var3) {
		if(this.drawButton) {
			FontRenderer var4 = var1.fontRenderer;
			GL11.glBindTexture(GL11.GL_TEXTURE_2D, var1.renderEngine.getTexture("/gui/gui.png"));
			GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
			boolean var5 = var2 >= this.xPosition && var3 >= this.yPosition && var2 < this.xPosition + this.field_52008_a && var3 < this.yPosition + this.field_52007_b;
			int var6 = this.getHoverState(var5);
			this.drawTexturedModalRect(this.xPosition, this.yPosition, 0, 46 + var6 * 20, this.field_52008_a / 2, this.field_52007_b);
			this.drawTexturedModalRect(this.xPosition + this.field_52008_a / 2, this.yPosition, 200 - this.field_52008_a / 2, 46 + var6 * 20, this.field_52008_a / 2, this.field_52007_b);
			this.mouseDragged(var1, var2, var3);
			int var7 = 14737632;
			if(!this.enabled) {
				var7 = -6250336;
			} else if(var5) {
				var7 = 16777120;
			}

			this.drawCenteredString(var4, this.displayString, this.xPosition + this.field_52008_a / 2, this.yPosition + (this.field_52007_b - 8) / 2, var7);
		}
	}

	protected void mouseDragged(Minecraft var1, int var2, int var3) {
	}

	public void mouseReleased(int var1, int var2) {
	}

	public boolean mousePressed(Minecraft var1, int var2, int var3) {
		return this.enabled && this.drawButton && var2 >= this.xPosition && var3 >= this.yPosition && var2 < this.xPosition + this.field_52008_a && var3 < this.yPosition + this.field_52007_b;
	}
}
