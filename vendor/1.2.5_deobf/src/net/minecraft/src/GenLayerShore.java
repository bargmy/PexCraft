package net.minecraft.src;

public class GenLayerShore extends GenLayer {
	public GenLayerShore(long var1, GenLayer var3) {
		super(var1);
		this.parent = var3;
	}

	public int[] getInts(int var1, int var2, int var3, int var4) {
		int[] var5 = this.parent.getInts(var1 - 1, var2 - 1, var3 + 2, var4 + 2);
		int[] var6 = IntCache.getIntCache(var3 * var4);

		for(int var7 = 0; var7 < var4; ++var7) {
			for(int var8 = 0; var8 < var3; ++var8) {
				this.initChunkSeed((long)(var8 + var1), (long)(var7 + var2));
				int var9 = var5[var8 + 1 + (var7 + 1) * (var3 + 2)];
				int var10;
				int var11;
				int var12;
				int var13;
				if(var9 == BiomeGenBase.mushroomIsland.biomeID) {
					var10 = var5[var8 + 1 + (var7 + 1 - 1) * (var3 + 2)];
					var11 = var5[var8 + 1 + 1 + (var7 + 1) * (var3 + 2)];
					var12 = var5[var8 + 1 - 1 + (var7 + 1) * (var3 + 2)];
					var13 = var5[var8 + 1 + (var7 + 1 + 1) * (var3 + 2)];
					if(var10 != BiomeGenBase.ocean.biomeID && var11 != BiomeGenBase.ocean.biomeID && var12 != BiomeGenBase.ocean.biomeID && var13 != BiomeGenBase.ocean.biomeID) {
						var6[var8 + var7 * var3] = var9;
					} else {
						var6[var8 + var7 * var3] = BiomeGenBase.mushroomIslandShore.biomeID;
					}
				} else if(var9 != BiomeGenBase.ocean.biomeID && var9 != BiomeGenBase.river.biomeID && var9 != BiomeGenBase.swampland.biomeID && var9 != BiomeGenBase.extremeHills.biomeID) {
					var10 = var5[var8 + 1 + (var7 + 1 - 1) * (var3 + 2)];
					var11 = var5[var8 + 1 + 1 + (var7 + 1) * (var3 + 2)];
					var12 = var5[var8 + 1 - 1 + (var7 + 1) * (var3 + 2)];
					var13 = var5[var8 + 1 + (var7 + 1 + 1) * (var3 + 2)];
					if(var10 != BiomeGenBase.ocean.biomeID && var11 != BiomeGenBase.ocean.biomeID && var12 != BiomeGenBase.ocean.biomeID && var13 != BiomeGenBase.ocean.biomeID) {
						var6[var8 + var7 * var3] = var9;
					} else {
						var6[var8 + var7 * var3] = BiomeGenBase.beach.biomeID;
					}
				} else if(var9 == BiomeGenBase.extremeHills.biomeID) {
					var10 = var5[var8 + 1 + (var7 + 1 - 1) * (var3 + 2)];
					var11 = var5[var8 + 1 + 1 + (var7 + 1) * (var3 + 2)];
					var12 = var5[var8 + 1 - 1 + (var7 + 1) * (var3 + 2)];
					var13 = var5[var8 + 1 + (var7 + 1 + 1) * (var3 + 2)];
					if(var10 == BiomeGenBase.extremeHills.biomeID && var11 == BiomeGenBase.extremeHills.biomeID && var12 == BiomeGenBase.extremeHills.biomeID && var13 == BiomeGenBase.extremeHills.biomeID) {
						var6[var8 + var7 * var3] = var9;
					} else {
						var6[var8 + var7 * var3] = BiomeGenBase.extremeHillsEdge.biomeID;
					}
				} else {
					var6[var8 + var7 * var3] = var9;
				}
			}
		}

		return var6;
	}
}
