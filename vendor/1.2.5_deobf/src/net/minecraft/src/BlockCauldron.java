package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class BlockCauldron extends Block {
	public BlockCauldron(int var1) {
		super(var1, Material.iron);
		this.blockIndexInTexture = 154;
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		return var1 == 1 ? 138 : (var1 == 0 ? 155 : 154);
	}

	public void getCollidingBoundingBoxes(World var1, int var2, int var3, int var4, AxisAlignedBB var5, ArrayList var6) {
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 5.0F / 16.0F, 1.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		float var7 = 2.0F / 16.0F;
		this.setBlockBounds(0.0F, 0.0F, 0.0F, var7, 1.0F, 1.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, var7);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		this.setBlockBounds(1.0F - var7, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		this.setBlockBounds(0.0F, 0.0F, 1.0F - var7, 1.0F, 1.0F, 1.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		this.setBlockBoundsForItemRender();
	}

	public void setBlockBoundsForItemRender() {
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
	}

	public boolean isOpaqueCube() {
		return false;
	}

	public int getRenderType() {
		return 24;
	}

	public boolean renderAsNormalBlock() {
		return false;
	}

	public boolean blockActivated(World var1, int var2, int var3, int var4, EntityPlayer var5) {
		if(var1.isRemote) {
			return true;
		} else {
			ItemStack var6 = var5.inventory.getCurrentItem();
			if(var6 == null) {
				return true;
			} else {
				int var7 = var1.getBlockMetadata(var2, var3, var4);
				if(var6.itemID == Item.bucketWater.shiftedIndex) {
					if(var7 < 3) {
						if(!var5.capabilities.isCreativeMode) {
							var5.inventory.setInventorySlotContents(var5.inventory.currentItem, new ItemStack(Item.bucketEmpty));
						}

						var1.setBlockMetadataWithNotify(var2, var3, var4, 3);
					}

					return true;
				} else {
					if(var6.itemID == Item.glassBottle.shiftedIndex && var7 > 0) {
						ItemStack var8 = new ItemStack(Item.potion, 1, 0);
						if(!var5.inventory.addItemStackToInventory(var8)) {
							var1.spawnEntityInWorld(new EntityItem(var1, (double)var2 + 0.5D, (double)var3 + 1.5D, (double)var4 + 0.5D, var8));
						}

						--var6.stackSize;
						if(var6.stackSize <= 0) {
							var5.inventory.setInventorySlotContents(var5.inventory.currentItem, (ItemStack)null);
						}

						var1.setBlockMetadataWithNotify(var2, var3, var4, var7 - 1);
					}

					return true;
				}
			}
		}
	}

	public int idDropped(int var1, Random var2, int var3) {
		return Item.cauldron.shiftedIndex;
	}
}
