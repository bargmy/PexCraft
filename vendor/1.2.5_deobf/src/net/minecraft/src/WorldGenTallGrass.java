package net.minecraft.src;

import java.util.Random;

public class WorldGenTallGrass extends WorldGenerator {
	private int tallGrassID;
	private int tallGrassMetadata;

	public WorldGenTallGrass(int var1, int var2) {
		this.tallGrassID = var1;
		this.tallGrassMetadata = var2;
	}

	public boolean generate(World var1, Random var2, int var3, int var4, int var5) {
		boolean var6 = false;

		while(true) {
			int var11 = var1.getBlockId(var3, var4, var5);
			if(var11 != 0 && var11 != Block.leaves.blockID || var4 <= 0) {
				for(int var7 = 0; var7 < 128; ++var7) {
					int var8 = var3 + var2.nextInt(8) - var2.nextInt(8);
					int var9 = var4 + var2.nextInt(4) - var2.nextInt(4);
					int var10 = var5 + var2.nextInt(8) - var2.nextInt(8);
					if(var1.isAirBlock(var8, var9, var10) && ((BlockFlower)Block.blocksList[this.tallGrassID]).canBlockStay(var1, var8, var9, var10)) {
						var1.setBlockAndMetadata(var8, var9, var10, this.tallGrassID, this.tallGrassMetadata);
					}
				}

				return true;
			}

			--var4;
		}
	}
}
