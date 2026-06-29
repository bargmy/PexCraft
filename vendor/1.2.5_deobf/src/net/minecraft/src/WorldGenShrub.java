package net.minecraft.src;

import java.util.Random;

public class WorldGenShrub extends WorldGenerator {
	private int field_48197_a;
	private int field_48196_b;

	public WorldGenShrub(int var1, int var2) {
		this.field_48196_b = var1;
		this.field_48197_a = var2;
	}

	public boolean generate(World var1, Random var2, int var3, int var4, int var5) {
		boolean var6 = false;

		while(true) {
			int var15 = var1.getBlockId(var3, var4, var5);
			if(var15 != 0 && var15 != Block.leaves.blockID || var4 <= 0) {
				int var7 = var1.getBlockId(var3, var4, var5);
				if(var7 == Block.dirt.blockID || var7 == Block.grass.blockID) {
					++var4;
					this.setBlockAndMetadata(var1, var3, var4, var5, Block.wood.blockID, this.field_48196_b);

					for(int var8 = var4; var8 <= var4 + 2; ++var8) {
						int var9 = var8 - var4;
						int var10 = 2 - var9;

						for(int var11 = var3 - var10; var11 <= var3 + var10; ++var11) {
							int var12 = var11 - var3;

							for(int var13 = var5 - var10; var13 <= var5 + var10; ++var13) {
								int var14 = var13 - var5;
								if((Math.abs(var12) != var10 || Math.abs(var14) != var10 || var2.nextInt(2) != 0) && !Block.opaqueCubeLookup[var1.getBlockId(var11, var8, var13)]) {
									this.setBlockAndMetadata(var1, var11, var8, var13, Block.leaves.blockID, this.field_48197_a);
								}
							}
						}
					}
				}

				return true;
			}

			--var4;
		}
	}
}
