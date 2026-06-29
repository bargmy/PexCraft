package net.minecraft.src;

public class BiomeCacheBlock {
	public float[] temperatureValues;
	public float[] rainfallValues;
	public BiomeGenBase[] biomes;
	public int xPosition;
	public int zPosition;
	public long lastAccessTime;
	final BiomeCache biomeCache;

	public BiomeCacheBlock(BiomeCache var1, int var2, int var3) {
		this.biomeCache = var1;
		this.temperatureValues = new float[256];
		this.rainfallValues = new float[256];
		this.biomes = new BiomeGenBase[256];
		this.xPosition = var2;
		this.zPosition = var3;
		BiomeCache.getChunkManager(var1).getTemperatures(this.temperatureValues, var2 << 4, var3 << 4, 16, 16);
		BiomeCache.getChunkManager(var1).getRainfall(this.rainfallValues, var2 << 4, var3 << 4, 16, 16);
		BiomeCache.getChunkManager(var1).getBiomeGenAt(this.biomes, var2 << 4, var3 << 4, 16, 16, false);
	}

	public BiomeGenBase getBiomeGenAt(int var1, int var2) {
		return this.biomes[var1 & 15 | (var2 & 15) << 4];
	}
}
