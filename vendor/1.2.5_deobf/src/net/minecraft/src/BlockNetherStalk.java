package net.minecraft.src;

import java.util.Random;

public class BlockNetherStalk extends BlockFlower {
	protected BlockNetherStalk(int var1) {
		super(var1, 226);
		this.setTickRandomly(true);
		float var2 = 0.5F;
		this.setBlockBounds(0.5F - var2, 0.0F, 0.5F - var2, 0.5F + var2, 0.25F, 0.5F + var2);
	}

	protected boolean canThisPlantGrowOnThisBlockID(int var1) {
		return var1 == Block.slowSand.blockID;
	}

	public boolean canBlockStay(World var1, int var2, int var3, int var4) {
		return this.canThisPlantGrowOnThisBlockID(var1.getBlockId(var2, var3 - 1, var4));
	}

	public void updateTick(World var1, int var2, int var3, int var4, Random var5) {
		int var6 = var1.getBlockMetadata(var2, var3, var4);
		if(var6 < 3) {
			BiomeGenBase var7 = var1.getBiomeGenForCoords(var2, var4);
			if(var7 instanceof BiomeGenHell && var5.nextInt(10) == 0) {
				++var6;
				var1.setBlockMetadataWithNotify(var2, var3, var4, var6);
			}
		}

		super.updateTick(var1, var2, var3, var4, var5);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		return var2 >= 3 ? this.blockIndexInTexture + 2 : (var2 > 0 ? this.blockIndexInTexture + 1 : this.blockIndexInTexture);
	}

	public int getRenderType() {
		return 6;
	}

	public void dropBlockAsItemWithChance(World var1, int var2, int var3, int var4, int var5, float var6, int var7) {
		if(!var1.isRemote) {
			int var8 = 1;
			if(var5 >= 3) {
				var8 = 2 + var1.rand.nextInt(3);
				if(var7 > 0) {
					var8 += var1.rand.nextInt(var7 + 1);
				}
			}

			for(int var9 = 0; var9 < var8; ++var9) {
				this.dropBlockAsItem_do(var1, var2, var3, var4, new ItemStack(Item.netherStalkSeeds));
			}

		}
	}

	public int idDropped(int var1, Random var2, int var3) {
		return 0;
	}

	public int quantityDropped(Random var1) {
		return 0;
	}
}
