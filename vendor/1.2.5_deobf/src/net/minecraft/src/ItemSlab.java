package net.minecraft.src;

public class ItemSlab extends ItemBlock {
	public ItemSlab(int var1) {
		super(var1);
		this.setMaxDamage(0);
		this.setHasSubtypes(true);
	}

	public int getIconFromDamage(int var1) {
		return Block.stairSingle.getBlockTextureFromSideAndMetadata(2, var1);
	}

	public int getMetadata(int var1) {
		return var1;
	}

	public String getItemNameIS(ItemStack var1) {
		int var2 = var1.getItemDamage();
		if(var2 < 0 || var2 >= BlockStep.blockStepTypes.length) {
			var2 = 0;
		}

		return super.getItemName() + "." + BlockStep.blockStepTypes[var2];
	}

	public boolean onItemUse(ItemStack var1, EntityPlayer var2, World var3, int var4, int var5, int var6, int var7) {
		if(var1.stackSize == 0) {
			return false;
		} else if(!var2.canPlayerEdit(var4, var5, var6)) {
			return false;
		} else {
			int var8 = var3.getBlockId(var4, var5, var6);
			int var9 = var3.getBlockMetadata(var4, var5, var6);
			int var10 = var9 & 7;
			boolean var11 = (var9 & 8) != 0;
			if((var7 == 1 && !var11 || var7 == 0 && var11) && var8 == Block.stairSingle.blockID && var10 == var1.getItemDamage()) {
				if(var3.checkIfAABBIsClear(Block.stairDouble.getCollisionBoundingBoxFromPool(var3, var4, var5, var6)) && var3.setBlockAndMetadataWithNotify(var4, var5, var6, Block.stairDouble.blockID, var10)) {
					var3.playSoundEffect((double)((float)var4 + 0.5F), (double)((float)var5 + 0.5F), (double)((float)var6 + 0.5F), Block.stairDouble.stepSound.getStepSound(), (Block.stairDouble.stepSound.getVolume() + 1.0F) / 2.0F, Block.stairDouble.stepSound.getPitch() * 0.8F);
					--var1.stackSize;
				}

				return true;
			} else {
				return func_50087_b(var1, var2, var3, var4, var5, var6, var7) ? true : super.onItemUse(var1, var2, var3, var4, var5, var6, var7);
			}
		}
	}

	private static boolean func_50087_b(ItemStack var0, EntityPlayer var1, World var2, int var3, int var4, int var5, int var6) {
		if(var6 == 0) {
			--var4;
		}

		if(var6 == 1) {
			++var4;
		}

		if(var6 == 2) {
			--var5;
		}

		if(var6 == 3) {
			++var5;
		}

		if(var6 == 4) {
			--var3;
		}

		if(var6 == 5) {
			++var3;
		}

		int var7 = var2.getBlockId(var3, var4, var5);
		int var8 = var2.getBlockMetadata(var3, var4, var5);
		int var9 = var8 & 7;
		if(var7 == Block.stairSingle.blockID && var9 == var0.getItemDamage()) {
			if(var2.checkIfAABBIsClear(Block.stairDouble.getCollisionBoundingBoxFromPool(var2, var3, var4, var5)) && var2.setBlockAndMetadataWithNotify(var3, var4, var5, Block.stairDouble.blockID, var9)) {
				var2.playSoundEffect((double)((float)var3 + 0.5F), (double)((float)var4 + 0.5F), (double)((float)var5 + 0.5F), Block.stairDouble.stepSound.getStepSound(), (Block.stairDouble.stepSound.getVolume() + 1.0F) / 2.0F, Block.stairDouble.stepSound.getPitch() * 0.8F);
				--var0.stackSize;
			}

			return true;
		} else {
			return false;
		}
	}
}
