package net.minecraft.src;

public class BiomeGenPlains extends BiomeGenBase {
	protected BiomeGenPlains(int var1) {
		super(var1);
		this.biomeDecorator.treesPerChunk = -999;
		this.biomeDecorator.flowersPerChunk = 4;
		this.biomeDecorator.grassPerChunk = 10;
	}
}
