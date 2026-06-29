package net.minecraft.src;

import java.util.Random;

public class BiomeDecorator {
	protected World currentWorld;
	protected Random randomGenerator;
	protected int chunk_X;
	protected int chunk_Z;
	protected BiomeGenBase biome;
	protected WorldGenerator clayGen = new WorldGenClay(4);
	protected WorldGenerator sandGen = new WorldGenSand(7, Block.sand.blockID);
	protected WorldGenerator gravelAsSandGen = new WorldGenSand(6, Block.gravel.blockID);
	protected WorldGenerator dirtGen = new WorldGenMinable(Block.dirt.blockID, 32);
	protected WorldGenerator gravelGen = new WorldGenMinable(Block.gravel.blockID, 32);
	protected WorldGenerator coalGen = new WorldGenMinable(Block.oreCoal.blockID, 16);
	protected WorldGenerator ironGen = new WorldGenMinable(Block.oreIron.blockID, 8);
	protected WorldGenerator goldGen = new WorldGenMinable(Block.oreGold.blockID, 8);
	protected WorldGenerator redstoneGen = new WorldGenMinable(Block.oreRedstone.blockID, 7);
	protected WorldGenerator diamondGen = new WorldGenMinable(Block.oreDiamond.blockID, 7);
	protected WorldGenerator lapisGen = new WorldGenMinable(Block.oreLapis.blockID, 6);
	protected WorldGenerator plantYellowGen = new WorldGenFlowers(Block.plantYellow.blockID);
	protected WorldGenerator plantRedGen = new WorldGenFlowers(Block.plantRed.blockID);
	protected WorldGenerator mushroomBrownGen = new WorldGenFlowers(Block.mushroomBrown.blockID);
	protected WorldGenerator mushroomRedGen = new WorldGenFlowers(Block.mushroomRed.blockID);
	protected WorldGenerator bigMushroomGen = new WorldGenBigMushroom();
	protected WorldGenerator reedGen = new WorldGenReed();
	protected WorldGenerator cactusGen = new WorldGenCactus();
	protected WorldGenerator waterlilyGen = new WorldGenWaterlily();
	protected int waterlilyPerChunk = 0;
	protected int treesPerChunk = 0;
	protected int flowersPerChunk = 2;
	protected int grassPerChunk = 1;
	protected int deadBushPerChunk = 0;
	protected int mushroomsPerChunk = 0;
	protected int reedsPerChunk = 0;
	protected int cactiPerChunk = 0;
	protected int sandPerChunk = 1;
	protected int sandPerChunk2 = 3;
	protected int clayPerChunk = 1;
	protected int bigMushroomsPerChunk = 0;
	public boolean generateLakes = true;

	public BiomeDecorator(BiomeGenBase var1) {
		this.biome = var1;
	}

	public void decorate(World var1, Random var2, int var3, int var4) {
		if(this.currentWorld != null) {
			throw new RuntimeException("Already decorating!!");
		} else {
			this.currentWorld = var1;
			this.randomGenerator = var2;
			this.chunk_X = var3;
			this.chunk_Z = var4;
			this.decorate();
			this.currentWorld = null;
			this.randomGenerator = null;
		}
	}

	protected void decorate() {
		this.generateOres();

		int var1;
		int var2;
		int var3;
		for(var1 = 0; var1 < this.sandPerChunk2; ++var1) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.sandGen.generate(this.currentWorld, this.randomGenerator, var2, this.currentWorld.getTopSolidOrLiquidBlock(var2, var3), var3);
		}

		for(var1 = 0; var1 < this.clayPerChunk; ++var1) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.clayGen.generate(this.currentWorld, this.randomGenerator, var2, this.currentWorld.getTopSolidOrLiquidBlock(var2, var3), var3);
		}

		for(var1 = 0; var1 < this.sandPerChunk; ++var1) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.sandGen.generate(this.currentWorld, this.randomGenerator, var2, this.currentWorld.getTopSolidOrLiquidBlock(var2, var3), var3);
		}

		var1 = this.treesPerChunk;
		if(this.randomGenerator.nextInt(10) == 0) {
			++var1;
		}

		int var4;
		for(var2 = 0; var2 < var1; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			WorldGenerator var5 = this.biome.getRandomWorldGenForTrees(this.randomGenerator);
			var5.setScale(1.0D, 1.0D, 1.0D);
			var5.generate(this.currentWorld, this.randomGenerator, var3, this.currentWorld.getHeightValue(var3, var4), var4);
		}

		for(var2 = 0; var2 < this.bigMushroomsPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.bigMushroomGen.generate(this.currentWorld, this.randomGenerator, var3, this.currentWorld.getHeightValue(var3, var4), var4);
		}

		int var7;
		for(var2 = 0; var2 < this.flowersPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.randomGenerator.nextInt(128);
			var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.plantYellowGen.generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
			if(this.randomGenerator.nextInt(4) == 0) {
				var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
				var4 = this.randomGenerator.nextInt(128);
				var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
				this.plantRedGen.generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
			}
		}

		for(var2 = 0; var2 < this.grassPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.randomGenerator.nextInt(128);
			var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			WorldGenerator var6 = this.biome.func_48410_b(this.randomGenerator);
			var6.generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
		}

		for(var2 = 0; var2 < this.deadBushPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.randomGenerator.nextInt(128);
			var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			(new WorldGenDeadBush(Block.deadBush.blockID)).generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
		}

		for(var2 = 0; var2 < this.waterlilyPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;

			for(var7 = this.randomGenerator.nextInt(128); var7 > 0 && this.currentWorld.getBlockId(var3, var7 - 1, var4) == 0; --var7) {
			}

			this.waterlilyGen.generate(this.currentWorld, this.randomGenerator, var3, var7, var4);
		}

		for(var2 = 0; var2 < this.mushroomsPerChunk; ++var2) {
			if(this.randomGenerator.nextInt(4) == 0) {
				var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
				var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
				var7 = this.currentWorld.getHeightValue(var3, var4);
				this.mushroomBrownGen.generate(this.currentWorld, this.randomGenerator, var3, var7, var4);
			}

			if(this.randomGenerator.nextInt(8) == 0) {
				var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
				var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
				var7 = this.randomGenerator.nextInt(128);
				this.mushroomRedGen.generate(this.currentWorld, this.randomGenerator, var3, var7, var4);
			}
		}

		if(this.randomGenerator.nextInt(4) == 0) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.randomGenerator.nextInt(128);
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.mushroomBrownGen.generate(this.currentWorld, this.randomGenerator, var2, var3, var4);
		}

		if(this.randomGenerator.nextInt(8) == 0) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.randomGenerator.nextInt(128);
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.mushroomRedGen.generate(this.currentWorld, this.randomGenerator, var2, var3, var4);
		}

		for(var2 = 0; var2 < this.reedsPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			var7 = this.randomGenerator.nextInt(128);
			this.reedGen.generate(this.currentWorld, this.randomGenerator, var3, var7, var4);
		}

		for(var2 = 0; var2 < 10; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.randomGenerator.nextInt(128);
			var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.reedGen.generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
		}

		if(this.randomGenerator.nextInt(32) == 0) {
			var2 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var3 = this.randomGenerator.nextInt(128);
			var4 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			(new WorldGenPumpkin()).generate(this.currentWorld, this.randomGenerator, var2, var3, var4);
		}

		for(var2 = 0; var2 < this.cactiPerChunk; ++var2) {
			var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
			var4 = this.randomGenerator.nextInt(128);
			var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
			this.cactusGen.generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
		}

		if(this.generateLakes) {
			for(var2 = 0; var2 < 50; ++var2) {
				var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
				var4 = this.randomGenerator.nextInt(this.randomGenerator.nextInt(120) + 8);
				var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
				(new WorldGenLiquids(Block.waterMoving.blockID)).generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
			}

			for(var2 = 0; var2 < 20; ++var2) {
				var3 = this.chunk_X + this.randomGenerator.nextInt(16) + 8;
				var4 = this.randomGenerator.nextInt(this.randomGenerator.nextInt(this.randomGenerator.nextInt(112) + 8) + 8);
				var7 = this.chunk_Z + this.randomGenerator.nextInt(16) + 8;
				(new WorldGenLiquids(Block.lavaMoving.blockID)).generate(this.currentWorld, this.randomGenerator, var3, var4, var7);
			}
		}

	}

	protected void genStandardOre1(int var1, WorldGenerator var2, int var3, int var4) {
		for(int var5 = 0; var5 < var1; ++var5) {
			int var6 = this.chunk_X + this.randomGenerator.nextInt(16);
			int var7 = this.randomGenerator.nextInt(var4 - var3) + var3;
			int var8 = this.chunk_Z + this.randomGenerator.nextInt(16);
			var2.generate(this.currentWorld, this.randomGenerator, var6, var7, var8);
		}

	}

	protected void genStandardOre2(int var1, WorldGenerator var2, int var3, int var4) {
		for(int var5 = 0; var5 < var1; ++var5) {
			int var6 = this.chunk_X + this.randomGenerator.nextInt(16);
			int var7 = this.randomGenerator.nextInt(var4) + this.randomGenerator.nextInt(var4) + (var3 - var4);
			int var8 = this.chunk_Z + this.randomGenerator.nextInt(16);
			var2.generate(this.currentWorld, this.randomGenerator, var6, var7, var8);
		}

	}

	protected void generateOres() {
		this.genStandardOre1(20, this.dirtGen, 0, 128);
		this.genStandardOre1(10, this.gravelGen, 0, 128);
		this.genStandardOre1(20, this.coalGen, 0, 128);
		this.genStandardOre1(20, this.ironGen, 0, 64);
		this.genStandardOre1(2, this.goldGen, 0, 32);
		this.genStandardOre1(8, this.redstoneGen, 0, 16);
		this.genStandardOre1(1, this.diamondGen, 0, 16);
		this.genStandardOre2(1, this.lapisGen, 16, 16);
	}
}
