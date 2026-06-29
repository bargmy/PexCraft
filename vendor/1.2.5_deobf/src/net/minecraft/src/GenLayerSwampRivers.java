package net.minecraft.src;

public class GenLayerSwampRivers extends GenLayer {
	public GenLayerSwampRivers(long var1, GenLayer var3) {
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
				if(var9 == BiomeGenBase.swampland.biomeID && this.nextInt(6) == 0) {
					var6[var8 + var7 * var3] = BiomeGenBase.river.biomeID;
				} else if((var9 == BiomeGenBase.jungle.biomeID || var9 == BiomeGenBase.jungleHills.biomeID) && this.nextInt(8) == 0) {
					var6[var8 + var7 * var3] = BiomeGenBase.river.biomeID;
				} else {
					var6[var8 + var7 * var3] = var9;
				}
			}
		}

		return var6;
	}
}
