package net.minecraft.src;

class GuiChatConfirmLink extends GuiConfirmOpenLink {
	final ChatClickData field_50056_a;
	final GuiChat field_50055_b;

	GuiChatConfirmLink(GuiChat var1, GuiScreen var2, String var3, int var4, ChatClickData var5) {
		super(var2, var3, var4);
		this.field_50055_b = var1;
		this.field_50056_a = var5;
	}

	public void func_50052_d() {
		func_50050_a(this.field_50056_a.func_50088_a());
	}
}
