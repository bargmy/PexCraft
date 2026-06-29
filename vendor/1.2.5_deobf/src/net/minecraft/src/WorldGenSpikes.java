package net.minecraft.src;

import java.util.Random;

public class WorldGenSpikes extends WorldGenerator {
	private int replaceID;

	public WorldGenSpikes(int var1) {
		this.replaceID = var1;
	}

	public boolean generate(World var1, Random var2, int var3, int var4, int var5) {
		if(var1.isAirBlock(var3, var4, var5) && var1.getBlockId(var3, var4 - 1, var5) == this.replaceID) {
			int var6 = var2.nextInt(32) + 6;
			int var7 = var2.nextInt(4) + 1;

			int var8;
			int var9;
			int var10;
			int var11;
			for(var8 = var3 - var7; var8 <= var3 + var7; ++var8) {
				for(var9 = var5 - var7; var9 <= var5 + var7; ++var9) {
					var10 = var8 - var3;
					var11 = var9 - var5;
					if(var10 * var10 + var11 * var11 <= var7 * var7 + 1 && var1.getBlockId(var8, var4 - 1, var9) != this.replaceID) {
						return false;
					}
				}
			}

			for(var8 = var4; var8 < var4 + var6 && var8 < 128; ++var8) {
				for(var9 = var3 - var7; var9 <= var3 + var7; ++var9) {
					for(var10 = var5 - var7; var10 <= var5 + var7; ++var10) {
						var11 = var9 - var3;
						int var12 = var10 - var5;
						if(var11 * var11 + var12 * var12 <= var7 * var7 + 1) {
							var1.setBlockWithNotify(var9, var8, var10, Block.obsidian.blockID);
						}
					}
				}
			}

			EntityEnderCrystal var13 = new EntityEnderCrystal(var1);
			var13.setLocationAndAngles((double)((float)var3 + 0.5F), (double)(var4 + var6), (double)((float)var5 + 0.5F), var2.nextFloat() * 360.0F, 0.0F);
			var1.spawnEntityInWorld(var13);
			var1.setBlockWithNotify(var3, var4 + var6, var5, Block.bedrock.blockID);
			return true;
		} else {
			return false;
		}
	}
}
