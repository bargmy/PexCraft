package net.minecraft.src;

import java.util.List;

public class ItemBoat extends Item {
	public ItemBoat(int var1) {
		super(var1);
		this.maxStackSize = 1;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		float var4 = 1.0F;
		float var5 = var3.prevRotationPitch + (var3.rotationPitch - var3.prevRotationPitch) * var4;
		float var6 = var3.prevRotationYaw + (var3.rotationYaw - var3.prevRotationYaw) * var4;
		double var7 = var3.prevPosX + (var3.posX - var3.prevPosX) * (double)var4;
		double var9 = var3.prevPosY + (var3.posY - var3.prevPosY) * (double)var4 + 1.62D - (double)var3.yOffset;
		double var11 = var3.prevPosZ + (var3.posZ - var3.prevPosZ) * (double)var4;
		Vec3D var13 = Vec3D.createVector(var7, var9, var11);
		float var14 = MathHelper.cos(-var6 * ((float)Math.PI / 180.0F) - (float)Math.PI);
		float var15 = MathHelper.sin(-var6 * ((float)Math.PI / 180.0F) - (float)Math.PI);
		float var16 = -MathHelper.cos(-var5 * ((float)Math.PI / 180.0F));
		float var17 = MathHelper.sin(-var5 * ((float)Math.PI / 180.0F));
		float var18 = var15 * var16;
		float var20 = var14 * var16;
		double var21 = 5.0D;
		Vec3D var23 = var13.addVector((double)var18 * var21, (double)var17 * var21, (double)var20 * var21);
		MovingObjectPosition var24 = var2.rayTraceBlocks_do(var13, var23, true);
		if(var24 == null) {
			return var1;
		} else {
			Vec3D var25 = var3.getLook(var4);
			boolean var26 = false;
			float var27 = 1.0F;
			List var28 = var2.getEntitiesWithinAABBExcludingEntity(var3, var3.boundingBox.addCoord(var25.xCoord * var21, var25.yCoord * var21, var25.zCoord * var21).expand((double)var27, (double)var27, (double)var27));

			for(int var29 = 0; var29 < var28.size(); ++var29) {
				Entity var30 = (Entity)var28.get(var29);
				if(var30.canBeCollidedWith()) {
					float var31 = var30.getCollisionBorderSize();
					AxisAlignedBB var32 = var30.boundingBox.expand((double)var31, (double)var31, (double)var31);
					if(var32.isVecInside(var13)) {
						var26 = true;
					}
				}
			}

			if(var26) {
				return var1;
			} else {
				if(var24.typeOfHit == EnumMovingObjectType.TILE) {
					int var33 = var24.blockX;
					int var34 = var24.blockY;
					int var35 = var24.blockZ;
					if(!var2.isRemote) {
						if(var2.getBlockId(var33, var34, var35) == Block.snow.blockID) {
							--var34;
						}

						var2.spawnEntityInWorld(new EntityBoat(var2, (double)((float)var33 + 0.5F), (double)((float)var34 + 1.0F), (double)((float)var35 + 0.5F)));
					}

					if(!var3.capabilities.isCreativeMode) {
						--var1.stackSize;
					}
				}

				return var1;
			}
		}
	}
}
