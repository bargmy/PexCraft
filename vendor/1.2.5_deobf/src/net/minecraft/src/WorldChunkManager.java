package net.minecraft.src;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class WorldChunkManager {
	private GenLayer genBiomes;
	private GenLayer biomeIndexLayer;
	private BiomeCache biomeCache;
	private List biomesToSpawnIn;

	protected WorldChunkManager() {
		this.biomeCache = new BiomeCache(this);
		this.biomesToSpawnIn = new ArrayList();
		this.biomesToSpawnIn.add(BiomeGenBase.forest);
		this.biomesToSpawnIn.add(BiomeGenBase.plains);
		this.biomesToSpawnIn.add(BiomeGenBase.taiga);
		this.biomesToSpawnIn.add(BiomeGenBase.taigaHills);
		this.biomesToSpawnIn.add(BiomeGenBase.forestHills);
		this.biomesToSpawnIn.add(BiomeGenBase.jungle);
		this.biomesToSpawnIn.add(BiomeGenBase.jungleHills);
	}

	public WorldChunkManager(long var1, WorldType var3) {
		this();
		GenLayer[] var4 = GenLayer.func_48425_a(var1, var3);
		this.genBiomes = var4[0];
		this.biomeIndexLayer = var4[1];
	}

	public WorldChunkManager(World var1) {
		this(var1.getSeed(), var1.getWorldInfo().getTerrainType());
	}

	public List getBiomesToSpawnIn() {
		return this.biomesToSpawnIn;
	}

	public BiomeGenBase getBiomeGenAt(int var1, int var2) {
		return this.biomeCache.getBiomeGenAt(var1, var2);
	}

	public float[] getRainfall(float[] var1, int var2, int var3, int var4, int var5) {
		IntCache.resetIntCache();
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new float[var4 * var5];
		}

		int[] var6 = this.biomeIndexLayer.getInts(var2, var3, var4, var5);

		for(int var7 = 0; var7 < var4 * var5; ++var7) {
			float var8 = (float)BiomeGenBase.biomeList[var6[var7]].getIntRainfall() / 65536.0F;
			if(var8 > 1.0F) {
				var8 = 1.0F;
			}

			var1[var7] = var8;
		}

		return var1;
	}

	public float getTemperatureAtHeight(float var1, int var2) {
		return var1;
	}

	public float[] getTemperatures(float[] var1, int var2, int var3, int var4, int var5) {
		IntCache.resetIntCache();
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new float[var4 * var5];
		}

		int[] var6 = this.biomeIndexLayer.getInts(var2, var3, var4, var5);

		for(int var7 = 0; var7 < var4 * var5; ++var7) {
			float var8 = (float)BiomeGenBase.biomeList[var6[var7]].getIntTemperature() / 65536.0F;
			if(var8 > 1.0F) {
				var8 = 1.0F;
			}

			var1[var7] = var8;
		}

		return var1;
	}

	public BiomeGenBase[] getBiomesForGeneration(BiomeGenBase[] var1, int var2, int var3, int var4, int var5) {
		IntCache.resetIntCache();
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new BiomeGenBase[var4 * var5];
		}

		int[] var6 = this.genBiomes.getInts(var2, var3, var4, var5);

		for(int var7 = 0; var7 < var4 * var5; ++var7) {
			var1[var7] = BiomeGenBase.biomeList[var6[var7]];
		}

		return var1;
	}

	public BiomeGenBase[] loadBlockGeneratorData(BiomeGenBase[] var1, int var2, int var3, int var4, int var5) {
		return this.getBiomeGenAt(var1, var2, var3, var4, var5, true);
	}

	public BiomeGenBase[] getBiomeGenAt(BiomeGenBase[] var1, int var2, int var3, int var4, int var5, boolean var6) {
		IntCache.resetIntCache();
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new BiomeGenBase[var4 * var5];
		}

		if(var6 && var4 == 16 && var5 == 16 && (var2 & 15) == 0 && (var3 & 15) == 0) {
			BiomeGenBase[] var9 = this.biomeCache.getCachedBiomes(var2, var3);
			System.arraycopy(var9, 0, var1, 0, var4 * var5);
			return var1;
		} else {
			int[] var7 = this.biomeIndexLayer.getInts(var2, var3, var4, var5);

			for(int var8 = 0; var8 < var4 * var5; ++var8) {
				var1[var8] = BiomeGenBase.biomeList[var7[var8]];
			}

			return var1;
		}
	}

	public boolean areBiomesViable(int var1, int var2, int var3, List var4) {
		int var5 = var1 - var3 >> 2;
		int var6 = var2 - var3 >> 2;
		int var7 = var1 + var3 >> 2;
		int var8 = var2 + var3 >> 2;
		int var9 = var7 - var5 + 1;
		int var10 = var8 - var6 + 1;
		int[] var11 = this.genBiomes.getInts(var5, var6, var9, var10);

		for(int var12 = 0; var12 < var9 * var10; ++var12) {
			BiomeGenBase var13 = BiomeGenBase.biomeList[var11[var12]];
			if(!var4.contains(var13)) {
				return false;
			}
		}

		return true;
	}

	public ChunkPosition findBiomePosition(int var1, int var2, int var3, List var4, Random var5) {
		int var6 = var1 - var3 >> 2;
		int var7 = var2 - var3 >> 2;
		int var8 = var1 + var3 >> 2;
		int var9 = var2 + var3 >> 2;
		int var10 = var8 - var6 + 1;
		int var11 = var9 - var7 + 1;
		int[] var12 = this.genBiomes.getInts(var6, var7, var10, var11);
		ChunkPosition var13 = null;
		int var14 = 0;

		for(int var15 = 0; var15 < var12.length; ++var15) {
			int var16 = var6 + var15 % var10 << 2;
			int var17 = var7 + var15 / var10 << 2;
			BiomeGenBase var18 = BiomeGenBase.biomeList[var12[var15]];
			if(var4.contains(var18) && (var13 == null || var5.nextInt(var14 + 1) == 0)) {
				var13 = new ChunkPosition(var16, 0, var17);
				++var14;
			}
		}

		return var13;
	}

	public void cleanupCache() {
		this.biomeCache.cleanupCache();
	}
}
