package net.minecraft.src;

import org.lwjgl.input.Keyboard;

public class GuiScreenServerList extends GuiScreen {
	private static String field_52009_d = "";
	private final GuiScreen guiScreen;
	private final ServerNBTStorage serverListStorage;
	private GuiTextField serverTextField;

	public GuiScreenServerList(GuiScreen var1, ServerNBTStorage var2) {
		this.guiScreen = var1;
		this.serverListStorage = var2;
	}

	public void updateScreen() {
		this.serverTextField.updateCursorCounter();
	}

	public void initGui() {
		StringTranslate var1 = StringTranslate.getInstance();
		Keyboard.enableRepeatEvents(true);
		this.controlList.clear();
		this.controlList.add(new GuiButton(0, this.width / 2 - 100, this.height / 4 + 96 + 12, var1.translateKey("selectServer.select")));
		this.controlList.add(new GuiButton(1, this.width / 2 - 100, this.height / 4 + 120 + 12, var1.translateKey("gui.cancel")));
		this.serverTextField = new GuiTextField(this.fontRenderer, this.width / 2 - 100, 116, 200, 20);
		this.serverTextField.setMaxStringLength(128);
		this.serverTextField.func_50033_b(true);
		this.serverTextField.setText(field_52009_d);
		((GuiButton)this.controlList.get(0)).enabled = this.serverTextField.getText().length() > 0 && this.serverTextField.getText().split(":").length > 0;
	}

	public void onGuiClosed() {
		Keyboard.enableRepeatEvents(false);
		field_52009_d = this.serverTextField.getText();
	}

	protected void actionPerformed(GuiButton var1) {
		if(var1.enabled) {
			if(var1.id == 1) {
				this.guiScreen.confirmClicked(false, 0);
			} else if(var1.id == 0) {
				this.serverListStorage.host = this.serverTextField.getText();
				this.guiScreen.confirmClicked(true, 0);
			}

		}
	}

	protected void keyTyped(char var1, int var2) {
		this.serverTextField.func_50037_a(var1, var2);
		if(var1 == 28) {
			this.actionPerformed((GuiButton)this.controlList.get(0));
		}

		((GuiButton)this.controlList.get(0)).enabled = this.serverTextField.getText().length() > 0 && this.serverTextField.getText().split(":").length > 0;
	}

	protected void mouseClicked(int var1, int var2, int var3) {
		super.mouseClicked(var1, var2, var3);
		this.serverTextField.mouseClicked(var1, var2, var3);
	}

	public void drawScreen(int var1, int var2, float var3) {
		StringTranslate var4 = StringTranslate.getInstance();
		this.drawDefaultBackground();
		this.drawCenteredString(this.fontRenderer, var4.translateKey("selectServer.direct"), this.width / 2, this.height / 4 - 60 + 20, 16777215);
		this.drawString(this.fontRenderer, var4.translateKey("addServer.enterIp"), this.width / 2 - 100, 100, 10526880);
		this.serverTextField.drawTextBox();
		super.drawScreen(var1, var2, var3);
	}
}
