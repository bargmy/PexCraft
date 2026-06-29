package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class GuiTextField extends Gui {
	private final FontRenderer fontRenderer;
	private final int xPos;
	private final int yPos;
	private final int width;
	private final int height;
	private String text = "";
	private int maxStringLength = 32;
	private int cursorCounter;
	private boolean field_50044_j = true;
	private boolean field_50045_k = true;
	private boolean isFocused = false;
	private boolean field_50043_m = true;
	private int field_50041_n = 0;
	private int field_50042_o = 0;
	private int field_50048_p = 0;
	private int field_50047_q = 14737632;
	private int field_50046_r = 7368816;

	public GuiTextField(FontRenderer var1, int var2, int var3, int var4, int var5) {
		this.fontRenderer = var1;
		this.xPos = var2;
		this.yPos = var3;
		this.width = var4;
		this.height = var5;
	}

	public void updateCursorCounter() {
		++this.cursorCounter;
	}

	public void setText(String var1) {
		if(var1.length() > this.maxStringLength) {
			this.text = var1.substring(0, this.maxStringLength);
		} else {
			this.text = var1;
		}

		this.func_50038_e();
	}

	public String getText() {
		return this.text;
	}

	public String func_50039_c() {
		int var1 = this.field_50042_o < this.field_50048_p ? this.field_50042_o : this.field_50048_p;
		int var2 = this.field_50042_o < this.field_50048_p ? this.field_50048_p : this.field_50042_o;
		return this.text.substring(var1, var2);
	}

	public void func_50031_b(String var1) {
		String var2 = "";
		String var3 = ChatAllowedCharacters.func_52019_a(var1);
		int var4 = this.field_50042_o < this.field_50048_p ? this.field_50042_o : this.field_50048_p;
		int var5 = this.field_50042_o < this.field_50048_p ? this.field_50048_p : this.field_50042_o;
		int var6 = this.maxStringLength - this.text.length() - (var4 - this.field_50048_p);
		boolean var7 = false;
		if(this.text.length() > 0) {
			var2 = var2 + this.text.substring(0, var4);
		}

		int var8;
		if(var6 < var3.length()) {
			var2 = var2 + var3.substring(0, var6);
			var8 = var6;
		} else {
			var2 = var2 + var3;
			var8 = var3.length();
		}

		if(this.text.length() > 0 && var5 < this.text.length()) {
			var2 = var2 + this.text.substring(var5);
		}

		this.text = var2;
		this.func_50023_d(var4 - this.field_50048_p + var8);
	}

	public void func_50021_a(int var1) {
		if(this.text.length() != 0) {
			if(this.field_50048_p != this.field_50042_o) {
				this.func_50031_b("");
			} else {
				this.func_50020_b(this.func_50028_c(var1) - this.field_50042_o);
			}
		}
	}

	public void func_50020_b(int var1) {
		if(this.text.length() != 0) {
			if(this.field_50048_p != this.field_50042_o) {
				this.func_50031_b("");
			} else {
				boolean var2 = var1 < 0;
				int var3 = var2 ? this.field_50042_o + var1 : this.field_50042_o;
				int var4 = var2 ? this.field_50042_o : this.field_50042_o + var1;
				String var5 = "";
				if(var3 >= 0) {
					var5 = this.text.substring(0, var3);
				}

				if(var4 < this.text.length()) {
					var5 = var5 + this.text.substring(var4);
				}

				this.text = var5;
				if(var2) {
					this.func_50023_d(var1);
				}

			}
		}
	}

	public int func_50028_c(int var1) {
		return this.func_50024_a(var1, this.func_50035_h());
	}

	public int func_50024_a(int var1, int var2) {
		int var3 = var2;
		boolean var4 = var1 < 0;
		int var5 = Math.abs(var1);

		for(int var6 = 0; var6 < var5; ++var6) {
			if(!var4) {
				int var7 = this.text.length();
				var3 = this.text.indexOf(32, var3);
				if(var3 == -1) {
					var3 = var7;
				} else {
					while(var3 < var7 && this.text.charAt(var3) == 32) {
						++var3;
					}
				}
			} else {
				while(var3 > 0 && this.text.charAt(var3 - 1) == 32) {
					--var3;
				}

				while(var3 > 0 && this.text.charAt(var3 - 1) != 32) {
					--var3;
				}
			}
		}

		return var3;
	}

	public void func_50023_d(int var1) {
		this.func_50030_e(this.field_50048_p + var1);
	}

	public void func_50030_e(int var1) {
		this.field_50042_o = var1;
		int var2 = this.text.length();
		if(this.field_50042_o < 0) {
			this.field_50042_o = 0;
		}

		if(this.field_50042_o > var2) {
			this.field_50042_o = var2;
		}

		this.func_50032_g(this.field_50042_o);
	}

	public void func_50034_d() {
		this.func_50030_e(0);
	}

	public void func_50038_e() {
		this.func_50030_e(this.text.length());
	}

	public boolean func_50037_a(char var1, int var2) {
		if(this.field_50043_m && this.isFocused) {
			switch(var1) {
			case '\u0001':
				this.func_50038_e();
				this.func_50032_g(0);
				return true;
			case '\u0003':
				GuiScreen.func_50050_a(this.func_50039_c());
				return true;
			case '\u0016':
				this.func_50031_b(GuiScreen.getClipboardString());
				return true;
			case '\u0018':
				GuiScreen.func_50050_a(this.func_50039_c());
				this.func_50031_b("");
				return true;
			default:
				switch(var2) {
				case 14:
					if(GuiScreen.func_50051_l()) {
						this.func_50021_a(-1);
					} else {
						this.func_50020_b(-1);
					}

					return true;
				case 199:
					if(GuiScreen.func_50049_m()) {
						this.func_50032_g(0);
					} else {
						this.func_50034_d();
					}

					return true;
				case 203:
					if(GuiScreen.func_50049_m()) {
						if(GuiScreen.func_50051_l()) {
							this.func_50032_g(this.func_50024_a(-1, this.func_50036_k()));
						} else {
							this.func_50032_g(this.func_50036_k() - 1);
						}
					} else if(GuiScreen.func_50051_l()) {
						this.func_50030_e(this.func_50028_c(-1));
					} else {
						this.func_50023_d(-1);
					}

					return true;
				case 205:
					if(GuiScreen.func_50049_m()) {
						if(GuiScreen.func_50051_l()) {
							this.func_50032_g(this.func_50024_a(1, this.func_50036_k()));
						} else {
							this.func_50032_g(this.func_50036_k() + 1);
						}
					} else if(GuiScreen.func_50051_l()) {
						this.func_50030_e(this.func_50028_c(1));
					} else {
						this.func_50023_d(1);
					}

					return true;
				case 207:
					if(GuiScreen.func_50049_m()) {
						this.func_50032_g(this.text.length());
					} else {
						this.func_50038_e();
					}

					return true;
				case 211:
					if(GuiScreen.func_50051_l()) {
						this.func_50021_a(1);
					} else {
						this.func_50020_b(1);
					}

					return true;
				default:
					if(ChatAllowedCharacters.isAllowedCharacter(var1)) {
						this.func_50031_b(Character.toString(var1));
						return true;
					} else {
						return false;
					}
				}
			}
		} else {
			return false;
		}
	}

	public void mouseClicked(int var1, int var2, int var3) {
		boolean var4 = var1 >= this.xPos && var1 < this.xPos + this.width && var2 >= this.yPos && var2 < this.yPos + this.height;
		if(this.field_50045_k) {
			this.func_50033_b(this.field_50043_m && var4);
		}

		if(this.isFocused && var3 == 0) {
			int var5 = var1 - this.xPos;
			if(this.field_50044_j) {
				var5 -= 4;
			}

			String var6 = this.fontRenderer.func_50107_a(this.text.substring(this.field_50041_n), this.func_50019_l());
			this.func_50030_e(this.fontRenderer.func_50107_a(var6, var5).length() + this.field_50041_n);
		}

	}

	public void drawTextBox() {
		if(this.func_50022_i()) {
			drawRect(this.xPos - 1, this.yPos - 1, this.xPos + this.width + 1, this.yPos + this.height + 1, -6250336);
			drawRect(this.xPos, this.yPos, this.xPos + this.width, this.yPos + this.height, -16777216);
		}

		int var1 = this.field_50043_m ? this.field_50047_q : this.field_50046_r;
		int var2 = this.field_50042_o - this.field_50041_n;
		int var3 = this.field_50048_p - this.field_50041_n;
		String var4 = this.fontRenderer.func_50107_a(this.text.substring(this.field_50041_n), this.func_50019_l());
		boolean var5 = var2 >= 0 && var2 <= var4.length();
		boolean var6 = this.isFocused && this.cursorCounter / 6 % 2 == 0 && var5;
		int var7 = this.field_50044_j ? this.xPos + 4 : this.xPos;
		int var8 = this.field_50044_j ? this.yPos + (this.height - 8) / 2 : this.yPos;
		int var9 = var7;
		if(var3 > var4.length()) {
			var3 = var4.length();
		}

		if(var4.length() > 0) {
			String var10 = var5 ? var4.substring(0, var2) : var4;
			var9 = this.fontRenderer.drawStringWithShadow(var10, var7, var8, var1);
		}

		boolean var13 = this.field_50042_o < this.text.length() || this.text.length() >= this.func_50040_g();
		int var11 = var9;
		if(!var5) {
			var11 = var2 > 0 ? var7 + this.width : var7;
		} else if(var13) {
			var11 = var9 - 1;
			--var9;
		}

		if(var4.length() > 0 && var5 && var2 < var4.length()) {
			this.fontRenderer.drawStringWithShadow(var4.substring(var2), var9, var8, var1);
		}

		if(var6) {
			if(var13) {
				Gui.drawRect(var11, var8 - 1, var11 + 1, var8 + 1 + this.fontRenderer.FONT_HEIGHT, -3092272);
			} else {
				this.fontRenderer.drawStringWithShadow("_", var11, var8, var1);
			}
		}

		if(var3 != var2) {
			int var12 = var7 + this.fontRenderer.getStringWidth(var4.substring(0, var3));
			this.func_50029_c(var11, var8 - 1, var12 - 1, var8 + 1 + this.fontRenderer.FONT_HEIGHT);
		}

	}

	private void func_50029_c(int var1, int var2, int var3, int var4) {
		int var5;
		if(var1 < var3) {
			var5 = var1;
			var1 = var3;
			var3 = var5;
		}

		if(var2 < var4) {
			var5 = var2;
			var2 = var4;
			var4 = var5;
		}

		Tessellator var6 = Tessellator.instance;
		GL11.glColor4f(0.0F, 0.0F, 255.0F, 255.0F);
		GL11.glDisable(GL11.GL_TEXTURE_2D);
		GL11.glEnable(GL11.GL_COLOR_LOGIC_OP);
		GL11.glLogicOp(GL11.GL_OR_REVERSE);
		var6.startDrawingQuads();
		var6.addVertex((double)var1, (double)var4, 0.0D);
		var6.addVertex((double)var3, (double)var4, 0.0D);
		var6.addVertex((double)var3, (double)var2, 0.0D);
		var6.addVertex((double)var1, (double)var2, 0.0D);
		var6.draw();
		GL11.glDisable(GL11.GL_COLOR_LOGIC_OP);
		GL11.glEnable(GL11.GL_TEXTURE_2D);
	}

	public void setMaxStringLength(int var1) {
		this.maxStringLength = var1;
		if(this.text.length() > var1) {
			this.text = this.text.substring(0, var1);
		}

	}

	public int func_50040_g() {
		return this.maxStringLength;
	}

	public int func_50035_h() {
		return this.field_50042_o;
	}

	public boolean func_50022_i() {
		return this.field_50044_j;
	}

	public void func_50027_a(boolean var1) {
		this.field_50044_j = var1;
	}

	public void func_50033_b(boolean var1) {
		if(var1 && !this.isFocused) {
			this.cursorCounter = 0;
		}

		this.isFocused = var1;
	}

	public boolean func_50025_j() {
		return this.isFocused;
	}

	public int func_50036_k() {
		return this.field_50048_p;
	}

	public int func_50019_l() {
		return this.func_50022_i() ? this.width - 8 : this.width;
	}

	public void func_50032_g(int var1) {
		int var2 = this.text.length();
		if(var1 > var2) {
			var1 = var2;
		}

		if(var1 < 0) {
			var1 = 0;
		}

		this.field_50048_p = var1;
		if(this.fontRenderer != null) {
			if(this.field_50041_n > var2) {
				this.field_50041_n = var2;
			}

			int var3 = this.func_50019_l();
			String var4 = this.fontRenderer.func_50107_a(this.text.substring(this.field_50041_n), var3);
			int var5 = var4.length() + this.field_50041_n;
			if(var1 == this.field_50041_n) {
				this.field_50041_n -= this.fontRenderer.func_50104_a(this.text, var3, true).length();
			}

			if(var1 > var5) {
				this.field_50041_n += var1 - var5;
			} else if(var1 <= this.field_50041_n) {
				this.field_50041_n -= this.field_50041_n - var1;
			}

			if(this.field_50041_n < 0) {
				this.field_50041_n = 0;
			}

			if(this.field_50041_n > var2) {
				this.field_50041_n = var2;
			}
		}

	}

	public void func_50026_c(boolean var1) {
		this.field_50045_k = var1;
	}
}
