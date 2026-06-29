package net.minecraft.src;

public enum EnumRarity {
	common(15, "Common"),
	uncommon(14, "Uncommon"),
	rare(11, "Rare"),
	epic(13, "Epic");

	public final int nameColor;
	public final String field_40532_f;

	private EnumRarity(int var3, String var4) {
		this.nameColor = var3;
		this.field_40532_f = var4;
	}
}
