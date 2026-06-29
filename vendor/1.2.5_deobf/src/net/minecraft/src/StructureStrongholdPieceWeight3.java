package net.minecraft.src;

final class StructureStrongholdPieceWeight3 extends StructureStrongholdPieceWeight {
	StructureStrongholdPieceWeight3(Class var1, int var2, int var3) {
		super(var1, var2, var3);
	}

	public boolean canSpawnMoreStructuresOfType(int var1) {
		return super.canSpawnMoreStructuresOfType(var1) && var1 > 5;
	}
}
