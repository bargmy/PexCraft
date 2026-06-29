package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class BlockBrewingStand extends BlockContainer {
	private Random rand = new Random();

	public BlockBrewingStand(int var1) {
		super(var1, Material.iron);
		this.blockIndexInTexture = 157;
	}

	public boolean isOpaqueCube() {
		return false;
	}

	public int getRenderType() {
		return 25;
	}

	public TileEntity getBlockEntity() {
		return new TileEntityBrewingStand();
	}

	public boolean renderAsNormalBlock() {
		return false;
	}

	public void getCollidingBoundingBoxes(World var1, int var2, int var3, int var4, AxisAlignedBB var5, ArrayList var6) {
		this.setBlockBounds(7.0F / 16.0F, 0.0F, 7.0F / 16.0F, 9.0F / 16.0F, 14.0F / 16.0F, 9.0F / 16.0F);
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
		this.setBlockBoundsForItemRender();
		super.getCollidingBoundingBoxes(var1, var2, var3, var4, var5, var6);
	}

	public void setBlockBoundsForItemRender() {
		this.setBlockBounds(0.0F, 0.0F, 0.0F, 1.0F, 2.0F / 16.0F, 1.0F);
	}

	public boolean blockActivated(World var1, int var2, int var3, int var4, EntityPlayer var5) {
		if(var1.isRemote) {
			return true;
		} else {
			TileEntityBrewingStand var6 = (TileEntityBrewingStand)var1.getBlockTileEntity(var2, var3, var4);
			if(var6 != null) {
				var5.displayGUIBrewingStand(var6);
			}

			return true;
		}
	}

	public void randomDisplayTick(World var1, int var2, int var3, int var4, Random var5) {
		double var6 = (double)((float)var2 + 0.4F + var5.nextFloat() * 0.2F);
		double var8 = (double)((float)var3 + 0.7F + var5.nextFloat() * 0.3F);
		double var10 = (double)((float)var4 + 0.4F + var5.nextFloat() * 0.2F);
		var1.spawnParticle("smoke", var6, var8, var10, 0.0D, 0.0D, 0.0D);
	}

	public void onBlockRemoval(World var1, int var2, int var3, int var4) {
		TileEntity var5 = var1.getBlockTileEntity(var2, var3, var4);
		if(var5 != null && var5 instanceof TileEntityBrewingStand) {
			TileEntityBrewingStand var6 = (TileEntityBrewingStand)var5;

			for(int var7 = 0; var7 < var6.getSizeInventory(); ++var7) {
				ItemStack var8 = var6.getStackInSlot(var7);
				if(var8 != null) {
					float var9 = this.rand.nextFloat() * 0.8F + 0.1F;
					float var10 = this.rand.nextFloat() * 0.8F + 0.1F;
					float var11 = this.rand.nextFloat() * 0.8F + 0.1F;

					while(var8.stackSize > 0) {
						int var12 = this.rand.nextInt(21) + 10;
						if(var12 > var8.stackSize) {
							var12 = var8.stackSize;
						}

						var8.stackSize -= var12;
						EntityItem var13 = new EntityItem(var1, (double)((float)var2 + var9), (double)((float)var3 + var10), (double)((float)var4 + var11), new ItemStack(var8.itemID, var12, var8.getItemDamage()));
						float var14 = 0.05F;
						var13.motionX = (double)((float)this.rand.nextGaussian() * var14);
						var13.motionY = (double)((float)this.rand.nextGaussian() * var14 + 0.2F);
						var13.motionZ = (double)((float)this.rand.nextGaussian() * var14);
						var1.spawnEntityInWorld(var13);
					}
				}
			}
		}

		super.onBlockRemoval(var1, var2, var3, var4);
	}

	public int idDropped(int var1, Random var2, int var3) {
		return Item.brewingStand.shiftedIndex;
	}
}
