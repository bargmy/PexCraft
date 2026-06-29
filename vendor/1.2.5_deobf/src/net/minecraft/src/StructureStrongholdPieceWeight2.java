package net.minecraft.src;

final class StructureStrongholdPieceWeight2 extends StructureStrongholdPieceWeight {
	StructureStrongholdPieceWeight2(Class var1, int var2, int var3) {
		super(var1, var2, var3);
	}

	public boolean canSpawnMoreStructuresOfType(int var1) {
		return super.canSpawnMoreStructuresOfType(var1) && var1 > 4;
	}
}
