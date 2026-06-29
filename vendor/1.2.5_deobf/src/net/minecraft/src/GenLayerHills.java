package net.minecraft.src;

public class GenLayerHills extends GenLayer {
	public GenLayerHills(long var1, GenLayer var3) {
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
				if(this.nextInt(3) == 0) {
					int var10 = var9;
					if(var9 == BiomeGenBase.desert.biomeID) {
						var10 = BiomeGenBase.desertHills.biomeID;
					} else if(var9 == BiomeGenBase.forest.biomeID) {
						var10 = BiomeGenBase.forestHills.biomeID;
					} else if(var9 == BiomeGenBase.taiga.biomeID) {
						var10 = BiomeGenBase.taigaHills.biomeID;
					} else if(var9 == BiomeGenBase.plains.biomeID) {
						var10 = BiomeGenBase.forest.biomeID;
					} else if(var9 == BiomeGenBase.icePlains.biomeID) {
						var10 = BiomeGenBase.iceMountains.biomeID;
					} else if(var9 == BiomeGenBase.jungle.biomeID) {
						var10 = BiomeGenBase.jungleHills.biomeID;
					}

					if(var10 != var9) {
						int var11 = var5[var8 + 1 + (var7 + 1 - 1) * (var3 + 2)];
						int var12 = var5[var8 + 1 + 1 + (var7 + 1) * (var3 + 2)];
						int var13 = var5[var8 + 1 - 1 + (var7 + 1) * (var3 + 2)];
						int var14 = var5[var8 + 1 + (var7 + 1 + 1) * (var3 + 2)];
						if(var11 == var9 && var12 == var9 && var13 == var9 && var14 == var9) {
							var6[var8 + var7 * var3] = var10;
						} else {
							var6[var8 + var7 * var3] = var9;
						}
					} else {
						var6[var8 + var7 * var3] = var9;
					}
				} else {
					var6[var8 + var7 * var3] = var9;
				}
			}
		}

		return var6;
	}
}
