package net.minecraft.src;

import java.util.Random;

public class BlockStationary extends BlockFluid {
	protected BlockStationary(int var1, Material var2) {
		super(var1, var2);
		this.setTickRandomly(false);
		if(var2 == Material.lava) {
			this.setTickRandomly(true);
		}

	}

	public boolean getBlocksMovement(IBlockAccess var1, int var2, int var3, int var4) {
		return this.blockMaterial != Material.lava;
	}

	public void onNeighborBlockChange(World var1, int var2, int var3, int var4, int var5) {
		super.onNeighborBlockChange(var1, var2, var3, var4, var5);
		if(var1.getBlockId(var2, var3, var4) == this.blockID) {
			this.setNotStationary(var1, var2, var3, var4);
		}

	}

	private void setNotStationary(World var1, int var2, int var3, int var4) {
		int var5 = var1.getBlockMetadata(var2, var3, var4);
		var1.editingBlocks = true;
		var1.setBlockAndMetadata(var2, var3, var4, this.blockID - 1, var5);
		var1.markBlocksDirty(var2, var3, var4, var2, var3, var4);
		var1.scheduleBlockUpdate(var2, var3, var4, this.blockID - 1, this.tickRate());
		var1.editingBlocks = false;
	}

	public void updateTick(World var1, int var2, int var3, int var4, Random var5) {
		if(this.blockMaterial == Material.lava) {
			int var6 = var5.nextInt(3);
			int var7 = 0;

			while(true) {
				int var8;
				if(var7 >= var6) {
					if(var6 == 0) {
						var7 = var2;
						var8 = var4;

						for(int var9 = 0; var9 < 3; ++var9) {
							var2 = var7 + var5.nextInt(3) - 1;
							var4 = var8 + var5.nextInt(3) - 1;
							if(var1.isAirBlock(var2, var3 + 1, var4) && this.isFlammable(var1, var2, var3, var4)) {
								var1.setBlockWithNotify(var2, var3 + 1, var4, Block.fire.blockID);
							}
						}
					}
					break;
				}

				var2 += var5.nextInt(3) - 1;
				++var3;
				var4 += var5.nextInt(3) - 1;
				var8 = var1.getBlockId(var2, var3, var4);
				if(var8 == 0) {
					if(this.isFlammable(var1, var2 - 1, var3, var4) || this.isFlammable(var1, var2 + 1, var3, var4) || this.isFlammable(var1, var2, var3, var4 - 1) || this.isFlammable(var1, var2, var3, var4 + 1) || this.isFlammable(var1, var2, var3 - 1, var4) || this.isFlammable(var1, var2, var3 + 1, var4)) {
						var1.setBlockWithNotify(var2, var3, var4, Block.fire.blockID);
						return;
					}
				} else if(Block.blocksList[var8].blockMaterial.blocksMovement()) {
					return;
				}

				++var7;
			}
		}

	}

	private boolean isFlammable(World var1, int var2, int var3, int var4) {
		return var1.getBlockMaterial(var2, var3, var4).getCanBurn();
	}
}
