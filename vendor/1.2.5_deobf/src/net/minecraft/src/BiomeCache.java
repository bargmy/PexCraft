package net.minecraft.src;

import java.util.ArrayList;
import java.util.List;

public class BiomeCache {
	private final WorldChunkManager chunkManager;
	private long lastCleanupTime = 0L;
	private LongHashMap cacheMap = new LongHashMap();
	private List cache = new ArrayList();

	public BiomeCache(WorldChunkManager var1) {
		this.chunkManager = var1;
	}

	public BiomeCacheBlock getBiomeCacheBlock(int var1, int var2) {
		var1 >>= 4;
		var2 >>= 4;
		long var3 = (long)var1 & 4294967295L | ((long)var2 & 4294967295L) << 32;
		BiomeCacheBlock var5 = (BiomeCacheBlock)this.cacheMap.getValueByKey(var3);
		if(var5 == null) {
			var5 = new BiomeCacheBlock(this, var1, var2);
			this.cacheMap.add(var3, var5);
			this.cache.add(var5);
		}

		var5.lastAccessTime = System.currentTimeMillis();
		return var5;
	}

	public BiomeGenBase getBiomeGenAt(int var1, int var2) {
		return this.getBiomeCacheBlock(var1, var2).getBiomeGenAt(var1, var2);
	}

	public void cleanupCache() {
		long var1 = System.currentTimeMillis();
		long var3 = var1 - this.lastCleanupTime;
		if(var3 > 7500L || var3 < 0L) {
			this.lastCleanupTime = var1;

			for(int var5 = 0; var5 < this.cache.size(); ++var5) {
				BiomeCacheBlock var6 = (BiomeCacheBlock)this.cache.get(var5);
				long var7 = var1 - var6.lastAccessTime;
				if(var7 > 30000L || var7 < 0L) {
					this.cache.remove(var5--);
					long var9 = (long)var6.xPosition & 4294967295L | ((long)var6.zPosition & 4294967295L) << 32;
					this.cacheMap.remove(var9);
				}
			}
		}

	}

	public BiomeGenBase[] getCachedBiomes(int var1, int var2) {
		return this.getBiomeCacheBlock(var1, var2).biomes;
	}

	static WorldChunkManager getChunkManager(BiomeCache var0) {
		return var0.chunkManager;
	}
}
