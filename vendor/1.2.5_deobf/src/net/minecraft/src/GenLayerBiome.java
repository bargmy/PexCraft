package net.minecraft.src;

public class GenLayerBiome extends GenLayer {
	private BiomeGenBase[] allowedBiomes = new BiomeGenBase[]{BiomeGenBase.desert, BiomeGenBase.forest, BiomeGenBase.extremeHills, BiomeGenBase.swampland, BiomeGenBase.plains, BiomeGenBase.taiga, BiomeGenBase.jungle};

	public GenLayerBiome(long var1, GenLayer var3, WorldType var4) {
		super(var1);
		this.parent = var3;
		if(var4 == WorldType.DEFAULT_1_1) {
			this.allowedBiomes = new BiomeGenBase[]{BiomeGenBase.desert, BiomeGenBase.forest, BiomeGenBase.extremeHills, BiomeGenBase.swampland, BiomeGenBase.plains, BiomeGenBase.taiga};
		}

	}

	public int[] getInts(int var1, int var2, int var3, int var4) {
		int[] var5 = this.parent.getInts(var1, var2, var3, var4);
		int[] var6 = IntCache.getIntCache(var3 * var4);

		for(int var7 = 0; var7 < var4; ++var7) {
			for(int var8 = 0; var8 < var3; ++var8) {
				this.initChunkSeed((long)(var8 + var1), (long)(var7 + var2));
				int var9 = var5[var8 + var7 * var3];
				if(var9 == 0) {
					var6[var8 + var7 * var3] = 0;
				} else if(var9 == BiomeGenBase.mushroomIsland.biomeID) {
					var6[var8 + var7 * var3] = var9;
				} else if(var9 == 1) {
					var6[var8 + var7 * var3] = this.allowedBiomes[this.nextInt(this.allowedBiomes.length)].biomeID;
				} else {
					var6[var8 + var7 * var3] = BiomeGenBase.icePlains.biomeID;
				}
			}
		}

		return var6;
	}
}
