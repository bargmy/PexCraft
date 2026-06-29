package net.minecraft.src;

public class GenLayerRiverMix extends GenLayer {
	private GenLayer field_35512_b;
	private GenLayer field_35513_c;

	public GenLayerRiverMix(long var1, GenLayer var3, GenLayer var4) {
		super(var1);
		this.field_35512_b = var3;
		this.field_35513_c = var4;
	}

	public void initWorldGenSeed(long var1) {
		this.field_35512_b.initWorldGenSeed(var1);
		this.field_35513_c.initWorldGenSeed(var1);
		super.initWorldGenSeed(var1);
	}

	public int[] getInts(int var1, int var2, int var3, int var4) {
		int[] var5 = this.field_35512_b.getInts(var1, var2, var3, var4);
		int[] var6 = this.field_35513_c.getInts(var1, var2, var3, var4);
		int[] var7 = IntCache.getIntCache(var3 * var4);

		for(int var8 = 0; var8 < var3 * var4; ++var8) {
			if(var5[var8] == BiomeGenBase.ocean.biomeID) {
				var7[var8] = var5[var8];
			} else if(var6[var8] >= 0) {
				if(var5[var8] == BiomeGenBase.icePlains.biomeID) {
					var7[var8] = BiomeGenBase.frozenRiver.biomeID;
				} else if(var5[var8] != BiomeGenBase.mushroomIsland.biomeID && var5[var8] != BiomeGenBase.mushroomIslandShore.biomeID) {
					var7[var8] = var6[var8];
				} else {
					var7[var8] = BiomeGenBase.mushroomIslandShore.biomeID;
				}
			} else {
				var7[var8] = var5[var8];
			}
		}

		return var7;
	}
}
