package net.minecraft.src;

public class BiomeGenBeach extends BiomeGenBase {
	public BiomeGenBeach(int var1) {
		super(var1);
		this.spawnableCreatureList.clear();
		this.topBlock = (byte)Block.sand.blockID;
		this.fillerBlock = (byte)Block.sand.blockID;
		this.biomeDecorator.treesPerChunk = -999;
		this.biomeDecorator.deadBushPerChunk = 0;
		this.biomeDecorator.reedsPerChunk = 0;
		this.biomeDecorator.cactiPerChunk = 0;
	}
}
