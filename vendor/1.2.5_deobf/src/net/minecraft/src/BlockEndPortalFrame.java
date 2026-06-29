package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class BlockEndPortalFrame extends Block {
	public BlockEndPortalFrame(int var1) {
		super(var1, 159, Material.glass);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		return var1 == 1 ? this.blockIndexInTexture - 1 : (var1 == 0 ? this.blockIndexInTexture + 16 : this.blockIndexInTexture);
	}

	public boolean isOpaqueCube() {
		return false;
	}

	public int getRenderType() {
		return 26;
	}

	public void setBlockBoundsForItemRender() {
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 13.0F / 16.0F, 1.0F);
	}

	public void getCollidingBoundingBoxes(World var1, int var2, int var3, int var4, AxisAlignedBB var5, ArrayList var6) {
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 13.0F / 16.0F, 1.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		int var7 = var1.getBlockMetadata(var2, var3, var4);
		if(isEnderEyeInserted(var7)) {
			this.setBlockBounds(5.0F / 16.0F, 13.0F / 16.0F, 5.0F / 16.0F, 11.0F / 16.0F, 1.0F, 11.0F / 16.0F);
			super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		}

		this.setBlockBoundsForItemRender();
	}

	public static boolean isEnderEyeInserted(int var0) {
		return (var0 & 4) != 0;
	}

	public int idDropped(int var1, Random var2, int var3) {
		return 0;
	}

	public void onBlockPlacedBy(World var1, int var2, int var3, int var4, EntityLiving var5) {
		int var6 = ((MathHelper.floor_double((double)(var5.rotationYaw * 4.0F / 360.0F) + 0.5D) & 3) + 2) % 4;
		var1.setBlockMetadataWithNotify(var2, var3, var4, var6);
	}
}
