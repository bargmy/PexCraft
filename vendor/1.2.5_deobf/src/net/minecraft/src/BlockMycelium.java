package net.minecraft.src;

import java.util.Random;

public class BlockMycelium extends Block {
	protected BlockMycelium(int var1) {
		super(var1, Material.grass);
		this.blockIndexInTexture = 77;
		this.setTickRandomly(true);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		return var1 == 1 ? 78 : (var1 == 0 ? 2 : 77);
	}

	public int getBlockTexture(IBlockAccess var1, int var2, int var3, int var4, int var5) {
		if(var5 == 1) {
			return 78;
		} else if(var5 == 0) {
			return 2;
		} else {
			Material var6 = var1.getBlockMaterial(var2, var3 + 1, var4);
			return var6 != Material.snow && var6 != Material.craftedSnow ? 77 : 68;
		}
	}

	public void updateTick(World var1, int var2, int var3, int var4, Random var5) {
		if(!var1.isRemote) {
			if(var1.getBlockLightValue(var2, var3 + 1, var4) < 4 && Block.lightOpacity[var1.getBlockId(var2, var3 + 1, var4)] > 2) {
				var1.setBlockWithNotify(var2, var3, var4, Block.dirt.blockID);
			} else if(var1.getBlockLightValue(var2, var3 + 1, var4) >= 9) {
				for(int var6 = 0; var6 < 4; ++var6) {
					int var7 = var2 + var5.nextInt(3) - 1;
					int var8 = var3 + var5.nextInt(5) - 3;
					int var9 = var4 + var5.nextInt(3) - 1;
					int var10 = var1.getBlockId(var7, var8 + 1, var9);
					if(var1.getBlockId(var7, var8, var9) == Block.dirt.blockID && var1.getBlockLightValue(var7, var8 + 1, var9) >= 4 && Block.lightOpacity[var10] <= 2) {
						var1.setBlockWithNotify(var7, var8, var9, this.blockID);
					}
				}
			}

		}
	}

	public void randomDisplayTick(World var1, int var2, int var3, int var4, Random var5) {
		super.randomDisplayTick(var1, var2, var3, var4, var5);
		if(var5.nextInt(10) == 0) {
			var1.spawnParticle("townaura", (double)((float)var2 + var5.nextFloat()), (double)((float)var3 + 1.1F), (double)((float)var4 + var5.nextFloat()), 0.0D, 0.0D, 0.0D);
		}

	}

	public int idDropped(int var1, Random var2, int var3) {
		return Block.dirt.idDropped(0, var2, var3);
	}
}
