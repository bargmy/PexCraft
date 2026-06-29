package net.minecraft.src;

public class BlockLilyPad extends BlockFlower {
	protected BlockLilyPad(int var1, int var2) {
		super(var1, var2);
		float var3 = 0.5F;
		float var4 = 0.015625F;
		this.setBlockBounds(0.5F - var3, 0.0F, 0.5F - var3, 0.5F + var3, var4, 0.5F + var3);
	}

	public int getRenderType() {
		return 23;
	}

	public AxisAlignedBB getCollisionBoundingBoxFromPool(World var1, int var2, int var3, int var4) {
		return AxisAlignedBB.getBoundingBoxFromPool((double)var2 + this.minX, (double)var3 + this.minY, (double)var4 + this.minZ, (double)var2 + this.maxX, (double)var3 + this.maxY, (double)var4 + this.maxZ);
	}

	public int getBlockColor() {
		return 2129968;
	}

	public int getRenderColor(int var1) {
		return 2129968;
	}

	public boolean canPlaceBlockAt(World var1, int var2, int var3, int var4) {
		return super.canPlaceBlockAt(var1, var2, var3, var4);
	}

	public int colorMultiplier(IBlockAccess var1, int var2, int var3, int var4) {
		return 2129968;
	}

	protected boolean canThisPlantGrowOnThisBlockID(int var1) {
		return var1 == Block.waterStill.blockID;
	}

	public boolean canBlockStay(World var1, int var2, int var3, int var4) {
		return var3 >= 0 && var3 < 256 ? var1.getBlockMaterial(var2, var3 - 1, var4) == Material.water && var1.getBlockMetadata(var2, var3 - 1, var4) == 0 : false;
	}
}
