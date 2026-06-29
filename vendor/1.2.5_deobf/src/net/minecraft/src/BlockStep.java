package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class BlockStep extends Block {
	public static final String[] blockStepTypes = new String[]{"stone", "sand", "wood", "cobble", "brick", "smoothStoneBrick"};
	private boolean blockType;

	public BlockStep(int var1, boolean var2) {
		super(var1, 6, Material.rock);
		this.blockType = var2;
		if(!var2) {
			this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F);
		} else {
			opaqueCubeLookup[var1] = true;
		}

		this.setLightOpacity(255);
	}

	public void setBlockBoundsBasedOnState(IBlockAccess var1, int var2, int var3, int var4) {
		if(this.blockType) {
			this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
		} else {
			boolean var5 = (var1.getBlockMetadata(var2, var3, var4) & 8) != 0;
			if(var5) {
				this.setBlockBounds(0.0F, 0.5F, 0.0F, 1.0F, 1.0F, 1.0F);
			} else {
				this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F);
			}
		}

	}

	public void setBlockBoundsForItemRender() {
		if(this.blockType) {
			this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
		} else {
			this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F);
		}

	}

	public void getCollidingBoundingBoxes(World var1, int var2, int var3, int var4, AxisAlignedBB var5, ArrayList var6) {
		this.setBlockBoundsBasedOnState(var1, var2, var3, var4);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		int var3 = var2 & 7;
		return var3 == 0 ? (var1 <= 1 ? 6 : 5) : (var3 == 1 ? (var1 == 0 ? 208 : (var1 == 1 ? 176 : 192)) : (var3 == 2 ? 4 : (var3 == 3 ? 16 : (var3 == 4 ? Block.brick.blockIndexInTexture : (var3 == 5 ? Block.stoneBrick.blockIndexInTexture : 6)))));
	}

	public int getBlockTextureFromSide(int var1) {
		return this.getBlockTextureFromSideAndMetadata(var1, 0);
	}

	public boolean isOpaqueCube() {
		return this.blockType;
	}

	public void onBlockPlaced(World var1, int var2, int var3, int var4, int var5) {
		if(var5 == 0 && !this.blockType) {
			int var6 = var1.getBlockMetadata(var2, var3, var4) & 7;
			var1.setBlockMetadataWithNotify(var2, var3, var4, var6 | 8);
		}

	}

	public int idDropped(int var1, Random var2, int var3) {
		return Block.stairSingle.blockID;
	}

	public int quantityDropped(Random var1) {
		return this.blockType ? 2 : 1;
	}

	protected int damageDropped(int var1) {
		return var1 & 7;
	}

	public boolean renderAsNormalBlock() {
		return this.blockType;
	}

	public boolean shouldSideBeRendered(IBlockAccess var1, int var2, int var3, int var4, int var5) {
		if(this.blockType) {
			super.shouldSideBeRendered(var1, var2, var3, var4, var5);
		}

		if(var5 != 1 && var5 != 0 && !super.shouldSideBeRendered(var1, var2, var3, var4, var5)) {
			return false;
		} else {
			int var6 = var2 + Facing.offsetsXForSide[Facing.faceToSide[var5]];
			int var7 = var3 + Facing.offsetsYForSide[Facing.faceToSide[var5]];
			int var8 = var4 + Facing.offsetsZForSide[Facing.faceToSide[var5]];
			boolean var9 = (var1.getBlockMetadata(var6, var7, var8) & 8) != 0;
			return !var9 ? (var5 == 1 ? true : (var5 == 0 && super.shouldSideBeRendered(var1, var2, var3, var4, var5) ? true : var1.getBlockId(var2, var3, var4) != this.blockID || (var1.getBlockMetadata(var2, var3, var4) & 8) != 0)) : (var5 == 0 ? true : (var5 == 1 && super.shouldSideBeRendered(var1, var2, var3, var4, var5) ? true : var1.getBlockId(var2, var3, var4) != this.blockID || (var1.getBlockMetadata(var2, var3, var4) & 8) == 0));
		}
	}

	protected ItemStack createStackedBlock(int var1) {
		return new ItemStack(Block.stairSingle.blockID, 1, var1 & 7);
	}
}
