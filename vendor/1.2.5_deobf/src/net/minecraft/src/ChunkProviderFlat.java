package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ChunkProviderFlat implements IChunkProvider {
	private World worldObj;
	private Random random;
	private final boolean useStructures;
	private MapGenVillage villageGen = new MapGenVillage(1);

	public ChunkProviderFlat(World var1, long var2, boolean var4) {
		this.worldObj = var1;
		this.useStructures = var4;
		this.random = new Random(var2);
	}

	private void generate(byte[] var1) {
		int var2 = var1.length / 256;

		for(int var3 = 0; var3 < 16; ++var3) {
			for(int var4 = 0; var4 < 16; ++var4) {
				for(int var5 = 0; var5 < var2; ++var5) {
					int var6 = 0;
					if(var5 == 0) {
						var6 = Block.bedrock.blockID;
					} else if(var5 <= 2) {
						var6 = Block.dirt.blockID;
					} else if(var5 == 3) {
						var6 = Block.grass.blockID;
					}

					var1[var3 << 11 | var4 << 7 | var5] = (byte)var6;
				}
			}
		}

	}

	public Chunk loadChunk(int var1, int var2) {
		return this.provideChunk(var1, var2);
	}

	public Chunk provideChunk(int var1, int var2) {
		byte[] var3 = new byte[-Short.MIN_VALUE];
		this.generate(var3);
		Chunk var4 = new Chunk(this.worldObj, var3, var1, var2);
		if(this.useStructures) {
			this.villageGen.generate(this, this.worldObj, var1, var2, var3);
		}

		BiomeGenBase[] var5 = this.worldObj.getWorldChunkManager().loadBlockGeneratorData((BiomeGenBase[])null, var1 * 16, var2 * 16, 16, 16);
		byte[] var6 = var4.getBiomeArray();

		for(int var7 = 0; var7 < var6.length; ++var7) {
			var6[var7] = (byte)var5[var7].biomeID;
		}

		var4.generateSkylightMap();
		return var4;
	}

	public boolean chunkExists(int var1, int var2) {
		return true;
	}

	public void populate(IChunkProvider var1, int var2, int var3) {
		this.random.setSeed(this.worldObj.getSeed());
		long var4 = this.random.nextLong() / 2L * 2L + 1L;
		long var6 = this.random.nextLong() / 2L * 2L + 1L;
		this.random.setSeed((long)var2 * var4 + (long)var3 * var6 ^ this.worldObj.getSeed());
		if(this.useStructures) {
			this.villageGen.generateStructuresInChunk(this.worldObj, this.random, var2, var3);
		}

	}

	public boolean saveChunks(boolean var1, IProgressUpdate var2) {
		return true;
	}

	public boolean unload100OldestChunks() {
		return false;
	}

	public boolean canSave() {
		return true;
	}

	public String makeString() {
		return "FlatLevelSource";
	}

	public List getPossibleCreatures(EnumCreatureType var1, int var2, int var3, int var4) {
		BiomeGenBase var5 = this.worldObj.getBiomeGenForCoords(var2, var4);
		return var5 == null ? null : var5.getSpawnableList(var1);
	}

	public ChunkPosition findClosestStructure(World var1, String var2, int var3, int var4, int var5) {
		return null;
	}
}
