package net.minecraft.src;

public class MapGenMineshaft extends MapGenStructure {
	protected boolean canSpawnStructureAtCoords(int var1, int var2) {
		return this.rand.nextInt(100) == 0 && this.rand.nextInt(80) < Math.max(Math.abs(var1), Math.abs(var2));
	}

	protected StructureStart getStructureStart(int var1, int var2) {
		return new StructureMineshaftStart(this.worldObj, this.rand, var1, var2);
	}
}
