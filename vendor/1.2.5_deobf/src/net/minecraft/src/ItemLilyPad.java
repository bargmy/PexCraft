package net.minecraft.src;

public class ItemLilyPad extends ItemColored {
	public ItemLilyPad(int var1) {
		super(var1, false);
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		MovingObjectPosition var4 = this.getMovingObjectPositionFromPlayer(var2, var3, true);
		if(var4 == null) {
			return var1;
		} else {
			if(var4.typeOfHit == EnumMovingObjectType.TILE) {
				int var5 = var4.blockX;
				int var6 = var4.blockY;
				int var7 = var4.blockZ;
				if(!var2.canMineBlock(var3, var5, var6, var7)) {
					return var1;
				}

				if(!var3.canPlayerEdit(var5, var6, var7)) {
					return var1;
				}

				if(var2.getBlockMaterial(var5, var6, var7) == Material.water && var2.getBlockMetadata(var5, var6, var7) == 0 && var2.isAirBlock(var5, var6 + 1, var7)) {
					var2.setBlockWithNotify(var5, var6 + 1, var7, Block.waterlily.blockID);
					if(!var3.capabilities.isCreativeMode) {
						--var1.stackSize;
					}
				}
			}

			return var1;
		}
	}

	public int getColorFromDamage(int var1, int var2) {
		return Block.waterlily.getRenderColor(var1);
	}
}
