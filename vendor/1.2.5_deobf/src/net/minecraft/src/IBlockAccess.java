package net.minecraft.src;

public interface IBlockAccess {
	int getBlockId(int var1, int var2, int var3);

	TileEntity getBlockTileEntity(int var1, int var2, int var3);

	int getLightBrightnessForSkyBlocks(int var1, int var2, int var3, int var4);

	float getBrightness(int var1, int var2, int var3, int var4);

	float getLightBrightness(int var1, int var2, int var3);

	int getBlockMetadata(int var1, int var2, int var3);

	Material getBlockMaterial(int var1, int var2, int var3);

	boolean isBlockOpaqueCube(int var1, int var2, int var3);

	boolean isBlockNormalCube(int var1, int var2, int var3);

	boolean isAirBlock(int var1, int var2, int var3);

	BiomeGenBase getBiomeGenForCoords(int var1, int var2);

	int getHeight();

	boolean func_48452_a();
}
