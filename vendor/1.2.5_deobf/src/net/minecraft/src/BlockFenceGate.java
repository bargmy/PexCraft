package net.minecraft.src;

public class BlockFenceGate extends BlockDirectional {
	public BlockFenceGate(int var1, int var2) {
		super(var1, var2, Material.wood);
	}

	public boolean canPlaceBlockAt(World var1, int var2, int var3, int var4) {
		return !var1.getBlockMaterial(var2, var3 - 1, var4).isSolid() ? false : super.canPlaceBlockAt(var1, var2, var3, var4);
	}

	public AxisAlignedBB getCollisionBoundingBoxFromPool(World var1, int var2, int var3, int var4) {
		int var5 = var1.getBlockMetadata(var2, var3, var4);
		return isFenceGateOpen(var5) ? null : (var5 != 2 && var5 != 0 ? AxisAlignedBB.getBoundingBoxFromPool((double)((float)var2 + 6.0F / 16.0F), (double)var3, (double)var4, (double)((float)var2 + 10.0F / 16.0F), (double)((float)var3 + 1.5F), (double)(var4 + 1)) : AxisAlignedBB.getBoundingBoxFromPool((double)var2, (double)var3, (double)((float)var4 + 6.0F / 16.0F), (double)(var2 + 1), (double)((float)var3 + 1.5F), (double)((float)var4 + 10.0F / 16.0F)));
	}

	public void setBlockBoundsBasedOnState(IBlockAccess var1, int var2, int var3, int var4) {
		int var5 = getDirection(var1.getBlockMetadata(var2, var3, var4));
		if(var5 != 2 && var5 != 0) {
			this.setBlockBounds(6.0F / 16.0F, 0.0F, 0.0F, 10.0F / 16.0F, 1.0F, 1.0F);
		} else {
			this.setBlockBounds(0.0F, 0.0F, 6.0F / 16.0F, 1.0F, 1.0F, 10.0F / 16.0F);
		}

	}

	public boolean isOpaqueCube() {
		return false;
	}

	public boolean renderAsNormalBlock() {
		return false;
	}

	public boolean getBlocksMovement(IBlockAccess var1, int var2, int var3, int var4) {
		return isFenceGateOpen(var1.getBlockMetadata(var2, var3, var4));
	}

	public int getRenderType() {
		return 21;
	}

	public void onBlockPlacedBy(World var1, int var2, int var3, int var4, EntityLiving var5) {
		int var6 = (MathHelper.floor_double((double)(var5.rotationYaw * 4.0F / 360.0F) + 0.5D) & 3) % 4;
		var1.setBlockMetadataWithNotify(var2, var3, var4, var6);
	}

	public boolean blockActivated(World var1, int var2, int var3, int var4, EntityPlayer var5) {
		int var6 = var1.getBlockMetadata(var2, var3, var4);
		if(isFenceGateOpen(var6)) {
			var1.setBlockMetadataWithNotify(var2, var3, var4, var6 & -5);
		} else {
			int var7 = (MathHelper.floor_double((double)(var5.rotationYaw * 4.0F / 360.0F) + 0.5D) & 3) % 4;
			int var8 = getDirection(var6);
			if(var8 == (var7 + 2) % 4) {
				var6 = var7;
			}

			var1.setBlockMetadataWithNotify(var2, var3, var4, var6 | 4);
		}

		var1.playAuxSFXAtEntity(var5, 1003, var2, var3, var4, 0);
		return true;
	}

	public void onNeighborBlockChange(World var1, int var2, int var3, int var4, int var5) {
		if(!var1.isRemote) {
			int var6 = var1.getBlockMetadata(var2, var3, var4);
			boolean var7 = var1.isBlockIndirectlyGettingPowered(var2, var3, var4);
			if(var7 || var5 > 0 && Block.blocksList[var5].canProvidePower() || var5 == 0) {
				if(var7 && !isFenceGateOpen(var6)) {
					var1.setBlockMetadataWithNotify(var2, var3, var4, var6 | 4);
					var1.playAuxSFXAtEntity((EntityPlayer)null, 1003, var2, var3, var4, 0);
				} else if(!var7 && isFenceGateOpen(var6)) {
					var1.setBlockMetadataWithNotify(var2, var3, var4, var6 & -5);
					var1.playAuxSFXAtEntity((EntityPlayer)null, 1003, var2, var3, var4, 0);
				}
			}

		}
	}

	public static boolean isFenceGateOpen(int var0) {
		return (var0 & 4) != 0;
	}
}
