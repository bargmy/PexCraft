package net.minecraft.src;

public class ItemEnderEye extends Item {
	public ItemEnderEye(int var1) {
		super(var1);
	}

	public boolean onItemUse(ItemStack var1, EntityPlayer var2, World var3, int var4, int var5, int var6, int var7) {
		int var8 = var3.getBlockId(var4, var5, var6);
		int var9 = var3.getBlockMetadata(var4, var5, var6);
		if(var2.canPlayerEdit(var4, var5, var6) && var8 == Block.endPortalFrame.blockID && !BlockEndPortalFrame.isEnderEyeInserted(var9)) {
			if(var3.isRemote) {
				return true;
			} else {
				var3.setBlockMetadataWithNotify(var4, var5, var6, var9 + 4);
				--var1.stackSize;

				int var10;
				for(var10 = 0; var10 < 16; ++var10) {
					double var11 = (double)((float)var4 + (5.0F + itemRand.nextFloat() * 6.0F) / 16.0F);
					double var13 = (double)((float)var5 + 13.0F / 16.0F);
					double var15 = (double)((float)var6 + (5.0F + itemRand.nextFloat() * 6.0F) / 16.0F);
					double var17 = 0.0D;
					double var19 = 0.0D;
					double var21 = 0.0D;
					var3.spawnParticle("smoke", var11, var13, var15, var17, var19, var21);
				}

				var10 = var9 & 3;
				int var23 = 0;
				int var12 = 0;
				boolean var24 = false;
				boolean var14 = true;
				int var25 = Direction.enderEyeMetaToDirection[var10];

				int var16;
				int var18;
				int var20;
				int var26;
				int var27;
				for(var16 = -2; var16 <= 2; ++var16) {
					var26 = var4 + Direction.offsetX[var25] * var16;
					var18 = var6 + Direction.offsetZ[var25] * var16;
					var27 = var3.getBlockId(var26, var5, var18);
					if(var27 == Block.endPortalFrame.blockID) {
						var20 = var3.getBlockMetadata(var26, var5, var18);
						if(!BlockEndPortalFrame.isEnderEyeInserted(var20)) {
							var14 = false;
							break;
						}

						if(!var24) {
							var23 = var16;
							var12 = var16;
							var24 = true;
						} else {
							var12 = var16;
						}
					}
				}

				if(var14 && var12 == var23 + 2) {
					for(var16 = var23; var16 <= var12; ++var16) {
						var26 = var4 + Direction.offsetX[var25] * var16;
						var18 = var6 + Direction.offsetZ[var25] * var16;
						var26 += Direction.offsetX[var10] * 4;
						var18 += Direction.offsetZ[var10] * 4;
						var27 = var3.getBlockId(var26, var5, var18);
						var20 = var3.getBlockMetadata(var26, var5, var18);
						if(var27 != Block.endPortalFrame.blockID || !BlockEndPortalFrame.isEnderEyeInserted(var20)) {
							var14 = false;
							break;
						}
					}

					for(var16 = var23 - 1; var16 <= var12 + 1; var16 += 4) {
						for(var26 = 1; var26 <= 3; ++var26) {
							var18 = var4 + Direction.offsetX[var25] * var16;
							var27 = var6 + Direction.offsetZ[var25] * var16;
							var18 += Direction.offsetX[var10] * var26;
							var27 += Direction.offsetZ[var10] * var26;
							var20 = var3.getBlockId(var18, var5, var27);
							int var28 = var3.getBlockMetadata(var18, var5, var27);
							if(var20 != Block.endPortalFrame.blockID || !BlockEndPortalFrame.isEnderEyeInserted(var28)) {
								var14 = false;
								break;
							}
						}
					}

					if(var14) {
						for(var16 = var23; var16 <= var12; ++var16) {
							for(var26 = 1; var26 <= 3; ++var26) {
								var18 = var4 + Direction.offsetX[var25] * var16;
								var27 = var6 + Direction.offsetZ[var25] * var16;
								var18 += Direction.offsetX[var10] * var26;
								var27 += Direction.offsetZ[var10] * var26;
								var3.setBlockWithNotify(var18, var5, var27, Block.endPortal.blockID);
							}
						}
					}
				}

				return true;
			}
		} else {
			return false;
		}
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		MovingObjectPosition var4 = this.getMovingObjectPositionFromPlayer(var2, var3, false);
		if(var4 != null && var4.typeOfHit == EnumMovingObjectType.TILE) {
			int var5 = var2.getBlockId(var4.blockX, var4.blockY, var4.blockZ);
			if(var5 == Block.endPortalFrame.blockID) {
				return var1;
			}
		}

		if(!var2.isRemote) {
			ChunkPosition var7 = var2.findClosestStructure("Stronghold", (int)var3.posX, (int)var3.posY, (int)var3.posZ);
			if(var7 != null) {
				EntityEnderEye var6 = new EntityEnderEye(var2, var3.posX, var3.posY + 1.62D - (double)var3.yOffset, var3.posZ);
				var6.func_40090_a((double)var7.x, var7.y, (double)var7.z);
				var2.spawnEntityInWorld(var6);
				var2.playSoundAtEntity(var3, "random.bow", 0.5F, 0.4F / (itemRand.nextFloat() * 0.4F + 0.8F));
				var2.playAuxSFXAtEntity((EntityPlayer)null, 1002, (int)var3.posX, (int)var3.posY, (int)var3.posZ, 0);
				if(!var3.capabilities.isCreativeMode) {
					--var1.stackSize;
				}
			}
		}

		return var1;
	}
}
