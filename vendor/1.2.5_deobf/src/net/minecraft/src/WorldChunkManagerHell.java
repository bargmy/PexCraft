package net.minecraft.src;

import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class WorldChunkManagerHell extends WorldChunkManager {
	private BiomeGenBase biomeGenerator;
	private float hellTemperature;
	private float rainfall;

	public WorldChunkManagerHell(BiomeGenBase var1, float var2, float var3) {
		this.biomeGenerator = var1;
		this.hellTemperature = var2;
		this.rainfall = var3;
	}

	public BiomeGenBase getBiomeGenAt(int var1, int var2) {
		return this.biomeGenerator;
	}

	public BiomeGenBase[] getBiomesForGeneration(BiomeGenBase[] var1, int var2, int var3, int var4, int var5) {
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new BiomeGenBase[var4 * var5];
		}

		Arrays.fill(var1, 0, var4 * var5, this.biomeGenerator);
		return var1;
	}

	public float[] getTemperatures(float[] var1, int var2, int var3, int var4, int var5) {
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new float[var4 * var5];
		}

		Arrays.fill(var1, 0, var4 * var5, this.hellTemperature);
		return var1;
	}

	public float[] getRainfall(float[] var1, int var2, int var3, int var4, int var5) {
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new float[var4 * var5];
		}

		Arrays.fill(var1, 0, var4 * var5, this.rainfall);
		return var1;
	}

	public BiomeGenBase[] loadBlockGeneratorData(BiomeGenBase[] var1, int var2, int var3, int var4, int var5) {
		if(var1 == null || var1.length < var4 * var5) {
			var1 = new BiomeGenBase[var4 * var5];
		}

		Arrays.fill(var1, 0, var4 * var5, this.biomeGenerator);
		return var1;
	}

	public BiomeGenBase[] getBiomeGenAt(BiomeGenBase[] var1, int var2, int var3, int var4, int var5, boolean var6) {
		return this.loadBlockGeneratorData(var1, var2, var3, var4, var5);
	}

	public ChunkPosition findBiomePosition(int var1, int var2, int var3, List var4, Random var5) {
		return var4.contains(this.biomeGenerator) ? new ChunkPosition(var1 - var3 + var5.nextInt(var3 * 2 + 1), 0, var2 - var3 + var5.nextInt(var3 * 2 + 1)) : null;
	}

	public boolean areBiomesViable(int var1, int var2, int var3, List var4) {
		return var4.contains(this.biomeGenerator);
	}
}
