package net.minecraft.src;

class StructureNetherBridgePieceWeight {
	public Class field_40699_a;
	public final int field_40697_b;
	public int field_40698_c;
	public int field_40695_d;
	public boolean field_40696_e;

	public StructureNetherBridgePieceWeight(Class var1, int var2, int var3, boolean var4) {
		this.field_40699_a = var1;
		this.field_40697_b = var2;
		this.field_40695_d = var3;
		this.field_40696_e = var4;
	}

	public StructureNetherBridgePieceWeight(Class var1, int var2, int var3) {
		this(var1, var2, var3, false);
	}

	public boolean func_40693_a(int var1) {
		return this.field_40695_d == 0 || this.field_40698_c < this.field_40695_d;
	}

	public boolean func_40694_a() {
		return this.field_40695_d == 0 || this.field_40698_c < this.field_40695_d;
	}
}
