package net.minecraft.src;

import java.util.Random;

public class RandomPositionGenerator {
	private static Vec3D field_48624_a = Vec3D.createVectorHelper(0.0D, 0.0D, 0.0D);

	public static Vec3D func_48622_a(EntityCreature var0, int var1, int var2) {
		return func_48621_c(var0, var1, var2, (Vec3D)null);
	}

	public static Vec3D func_48620_a(EntityCreature var0, int var1, int var2, Vec3D var3) {
		field_48624_a.xCoord = var3.xCoord - var0.posX;
		field_48624_a.yCoord = var3.yCoord - var0.posY;
		field_48624_a.zCoord = var3.zCoord - var0.posZ;
		return func_48621_c(var0, var1, var2, field_48624_a);
	}

	public static Vec3D func_48623_b(EntityCreature var0, int var1, int var2, Vec3D var3) {
		field_48624_a.xCoord = var0.posX - var3.xCoord;
		field_48624_a.yCoord = var0.posY - var3.yCoord;
		field_48624_a.zCoord = var0.posZ - var3.zCoord;
		return func_48621_c(var0, var1, var2, field_48624_a);
	}

	private static Vec3D func_48621_c(EntityCreature var0, int var1, int var2, Vec3D var3) {
		Random var4 = var0.getRNG();
		boolean var5 = false;
		int var6 = 0;
		int var7 = 0;
		int var8 = 0;
		float var9 = -99999.0F;
		boolean var10;
		if(var0.hasHome()) {
			double var11 = var0.getHomePosition().getEuclideanDistanceTo(MathHelper.floor_double(var0.posX), MathHelper.floor_double(var0.posY), MathHelper.floor_double(var0.posZ)) + 4.0D;
			var10 = var11 < (double)(var0.getMaximumHomeDistance() + (float)var1);
		} else {
			var10 = false;
		}

		for(int var16 = 0; var16 < 10; ++var16) {
			int var12 = var4.nextInt(2 * var1) - var1;
			int var13 = var4.nextInt(2 * var2) - var2;
			int var14 = var4.nextInt(2 * var1) - var1;
			if(var3 == null || (double)var12 * var3.xCoord + (double)var14 * var3.zCoord >= 0.0D) {
				var12 += MathHelper.floor_double(var0.posX);
				var13 += MathHelper.floor_double(var0.posY);
				var14 += MathHelper.floor_double(var0.posZ);
				if(!var10 || var0.isWithinHomeDistance(var12, var13, var14)) {
					float var15 = var0.getBlockPathWeight(var12, var13, var14);
					if(var15 > var9) {
						var9 = var15;
						var6 = var12;
						var7 = var13;
						var8 = var14;
						var5 = true;
					}
				}
			}
		}

		if(var5) {
			return Vec3D.createVector((double)var6, (double)var7, (double)var8);
		} else {
			return null;
		}
	}
}
