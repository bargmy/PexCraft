package net.minecraft.src;

import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class MapGenVillage extends MapGenStructure {
	public static List villageSpawnBiomes = Arrays.asList(new BiomeGenBase[]{BiomeGenBase.plains, BiomeGenBase.desert});
	private final int terrainType;

	public MapGenVillage(int var1) {
		this.terrainType = var1;
	}

	protected boolean canSpawnStructureAtCoords(int var1, int var2) {
		byte var3 = 32;
		byte var4 = 8;
		int var5 = var1;
		int var6 = var2;
		if(var1 < 0) {
			var1 -= var3 - 1;
		}

		if(var2 < 0) {
			var2 -= var3 - 1;
		}

		int var7 = var1 / var3;
		int var8 = var2 / var3;
		Random var9 = this.worldObj.setRandomSeed(var7, var8, 10387312);
		var7 *= var3;
		var8 *= var3;
		var7 += var9.nextInt(var3 - var4);
		var8 += var9.nextInt(var3 - var4);
		if(var5 == var7 && var6 == var8) {
			boolean var10 = this.worldObj.getWorldChunkManager().areBiomesViable(var5 * 16 + 8, var6 * 16 + 8, 0, villageSpawnBiomes);
			if(var10) {
				return true;
			}
		}

		return false;
	}

	protected StructureStart getStructureStart(int var1, int var2) {
		return new StructureVillageStart(this.worldObj, this.rand, var1, var2, this.terrainType);
	}
}
