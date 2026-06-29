package net.minecraft.src;

import java.net.URI;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

public class GuiChat extends GuiScreen {
	private String field_50062_b = "";
	private int field_50063_c = -1;
	private boolean field_50060_d = false;
	private String field_50061_e = "";
	private String field_50059_f = "";
	private int field_50067_h = 0;
	private List field_50068_i = new ArrayList();
	private URI field_50065_j = null;
	protected GuiTextField field_50064_a;
	private String field_50066_k = "";

	public GuiChat() {
	}

	public GuiChat(String var1) {
		this.field_50066_k = var1;
	}

	public void initGui() {
		Keyboard.enableRepeatEvents(true);
		this.field_50063_c = this.mc.ingameGUI.func_50013_c().size();
		this.field_50064_a = new GuiTextField(this.fontRenderer, 4, this.height - 12, this.width - 4, 12);
		this.field_50064_a.setMaxStringLength(100);
		this.field_50064_a.func_50027_a(false);
		this.field_50064_a.func_50033_b(true);
		this.field_50064_a.setText(this.field_50066_k);
		this.field_50064_a.func_50026_c(false);
	}

	public void onGuiClosed() {
		Keyboard.enableRepeatEvents(false);
		this.mc.ingameGUI.func_50014_d();
	}

	public void updateScreen() {
		this.field_50064_a.updateCursorCounter();
	}

	protected void keyTyped(char var1, int var2) {
		if(var2 == 15) {
			this.completePlayerName();
		} else {
			this.field_50060_d = false;
		}

		if(var2 == 1) {
			this.mc.displayGuiScreen((GuiScreen)null);
		} else if(var2 == 28) {
			String var3 = this.field_50064_a.getText().trim();
			if(var3.length() > 0 && !this.mc.lineIsCommand(var3)) {
				this.mc.thePlayer.sendChatMessage(var3);
			}

			this.mc.displayGuiScreen((GuiScreen)null);
		} else if(var2 == 200) {
			this.func_50058_a(-1);
		} else if(var2 == 208) {
			this.func_50058_a(1);
		} else if(var2 == 201) {
			this.mc.ingameGUI.func_50011_a(19);
		} else if(var2 == 209) {
			this.mc.ingameGUI.func_50011_a(-19);
		} else {
			this.field_50064_a.func_50037_a(var1, var2);
		}

	}

	public void handleMouseInput() {
		super.handleMouseInput();
		int var1 = Mouse.getEventDWheel();
		if(var1 != 0) {
			if(var1 > 1) {
				var1 = 1;
			}

			if(var1 < -1) {
				var1 = -1;
			}

			if(!func_50049_m()) {
				var1 *= 7;
			}

			this.mc.ingameGUI.func_50011_a(var1);
		}

	}

	protected void mouseClicked(int var1, int var2, int var3) {
		if(var3 == 0) {
			ChatClickData var4 = this.mc.ingameGUI.func_50012_a(Mouse.getX(), Mouse.getY());
			if(var4 != null) {
				URI var5 = var4.func_50089_b();
				if(var5 != null) {
					this.field_50065_j = var5;
					this.mc.displayGuiScreen(new GuiChatConfirmLink(this, this, var4.func_50088_a(), 0, var4));
					return;
				}
			}
		}

		this.field_50064_a.mouseClicked(var1, var2, var3);
		super.mouseClicked(var1, var2, var3);
	}

	public void confirmClicked(boolean var1, int var2) {
		if(var2 == 0) {
			if(var1) {
				try {
					Class var3 = Class.forName("java.awt.Desktop");
					Object var4 = var3.getMethod("getDesktop", new Class[0]).invoke((Object)null, new Object[0]);
					var3.getMethod("browse", new Class[]{URI.class}).invoke(var4, new Object[]{this.field_50065_j});
				} catch (Throwable var5) {
					var5.printStackTrace();
				}
			}

			this.field_50065_j = null;
			this.mc.displayGuiScreen(this);
		}

	}

	public void completePlayerName() {
		Iterator var2;
		GuiPlayerInfo var3;
		if(this.field_50060_d) {
			this.field_50064_a.func_50021_a(-1);
			if(this.field_50067_h >= this.field_50068_i.size()) {
				this.field_50067_h = 0;
			}
		} else {
			int var1 = this.field_50064_a.func_50028_c(-1);
			if(this.field_50064_a.func_50035_h() - var1 < 1) {
				return;
			}

			this.field_50068_i.clear();
			this.field_50061_e = this.field_50064_a.getText().substring(var1);
			this.field_50059_f = this.field_50061_e.toLowerCase();
			var2 = ((EntityClientPlayerMP)this.mc.thePlayer).sendQueue.playerNames.iterator();

			while(var2.hasNext()) {
				var3 = (GuiPlayerInfo)var2.next();
				if(var3.nameStartsWith(this.field_50059_f)) {
					this.field_50068_i.add(var3);
				}
			}

			if(this.field_50068_i.size() == 0) {
				return;
			}

			this.field_50060_d = true;
			this.field_50067_h = 0;
			this.field_50064_a.func_50020_b(var1 - this.field_50064_a.func_50035_h());
		}

		if(this.field_50068_i.size() > 1) {
			StringBuilder var4 = new StringBuilder();

			for(var2 = this.field_50068_i.iterator(); var2.hasNext(); var4.append(var3.name)) {
				var3 = (GuiPlayerInfo)var2.next();
				if(var4.length() > 0) {
					var4.append(", ");
				}
			}

			this.mc.ingameGUI.addChatMessage(var4.toString());
		}

		this.field_50064_a.func_50031_b(((GuiPlayerInfo)this.field_50068_i.get(this.field_50067_h++)).name);
	}

	public void func_50058_a(int var1) {
		int var2 = this.field_50063_c + var1;
		int var3 = this.mc.ingameGUI.func_50013_c().size();
		if(var2 < 0) {
			var2 = 0;
		}

		if(var2 > var3) {
			var2 = var3;
		}

		if(var2 != this.field_50063_c) {
			if(var2 == var3) {
				this.field_50063_c = var3;
				this.field_50064_a.setText(this.field_50062_b);
			} else {
				if(this.field_50063_c == var3) {
					this.field_50062_b = this.field_50064_a.getText();
				}

				this.field_50064_a.setText((String)this.mc.ingameGUI.func_50013_c().get(var2));
				this.field_50063_c = var2;
			}
		}
	}

	public void drawScreen(int var1, int var2, float var3) {
		drawRect(2, this.height - 14, this.width - 2, this.height - 2, Integer.MIN_VALUE);
		this.field_50064_a.drawTextBox();
		super.drawScreen(var1, var2, var3);
	}
}
