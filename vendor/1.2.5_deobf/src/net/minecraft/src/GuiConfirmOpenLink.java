package net.minecraft.src;

public abstract class GuiConfirmOpenLink extends GuiYesNo {
	private String field_50054_a;
	private String field_50053_b;

	public GuiConfirmOpenLink(GuiScreen var1, String var2, int var3) {
		super(var1, StringTranslate.getInstance().translateKey("chat.link.confirm"), var2, var3);
		StringTranslate var4 = StringTranslate.getInstance();
		this.buttonText1 = var4.translateKey("gui.yes");
		this.buttonText2 = var4.translateKey("gui.no");
		this.field_50053_b = var4.translateKey("chat.copy");
		this.field_50054_a = var4.translateKey("chat.link.warning");
	}

	public void initGui() {
		this.controlList.add(new GuiButton(0, this.width / 3 - 83 + 0, this.height / 6 + 96, 100, 20, this.buttonText1));
		this.controlList.add(new GuiButton(2, this.width / 3 - 83 + 105, this.height / 6 + 96, 100, 20, this.field_50053_b));
		this.controlList.add(new GuiButton(1, this.width / 3 - 83 + 210, this.height / 6 + 96, 100, 20, this.buttonText2));
	}

	protected void actionPerformed(GuiButton var1) {
		if(var1.id == 2) {
			this.func_50052_d();
			super.actionPerformed((GuiButton)this.controlList.get(1));
		} else {
			super.actionPerformed(var1);
		}

	}

	public abstract void func_50052_d();

	public void drawScreen(int var1, int var2, float var3) {
		super.drawScreen(var1, var2, var3);
		this.drawCenteredString(this.fontRenderer, this.field_50054_a, this.width / 2, 110, 16764108);
	}
}
