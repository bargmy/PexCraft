package net.minecraft.src;

public class PathNavigate {
	private EntityLiving theEntity;
	private World worldObj;
	private PathEntity currentPath;
	private float speed;
	private float pathSearchRange;
	private boolean noSunPathfind = false;
	private int totalTicks;
	private int ticksAtLastPos;
	private Vec3D lastPosCheck = Vec3D.createVectorHelper(0.0D, 0.0D, 0.0D);
	private boolean canPassOpenWoodenDoors = true;
	private boolean canPassClosedWoodenDoors = false;
	private boolean avoidsWater = false;
	private boolean canSwim = false;

	public PathNavigate(EntityLiving var1, World var2, float var3) {
		this.theEntity = var1;
		this.worldObj = var2;
		this.pathSearchRange = var3;
	}

	public void func_48664_a(boolean var1) {
		this.avoidsWater = var1;
	}

	public boolean func_48658_a() {
		return this.avoidsWater;
	}

	public void setBreakDoors(boolean var1) {
		this.canPassClosedWoodenDoors = var1;
	}

	public void func_48663_c(boolean var1) {
		this.canPassOpenWoodenDoors = var1;
	}

	public boolean func_48665_b() {
		return this.canPassClosedWoodenDoors;
	}

	public void func_48680_d(boolean var1) {
		this.noSunPathfind = var1;
	}

	public void setSpeed(float var1) {
		this.speed = var1;
	}

	public void func_48669_e(boolean var1) {
		this.canSwim = var1;
	}

	public PathEntity getPathToXYZ(double var1, double var3, double var5) {
		return !this.canNavigate() ? null : this.worldObj.getEntityPathToXYZ(this.theEntity, MathHelper.floor_double(var1), (int)var3, MathHelper.floor_double(var5), this.pathSearchRange, this.canPassOpenWoodenDoors, this.canPassClosedWoodenDoors, this.avoidsWater, this.canSwim);
	}

	public boolean func_48666_a(double var1, double var3, double var5, float var7) {
		PathEntity var8 = this.getPathToXYZ((double)MathHelper.floor_double(var1), (double)((int)var3), (double)MathHelper.floor_double(var5));
		return this.setPath(var8, var7);
	}

	public PathEntity func_48679_a(EntityLiving var1) {
		return !this.canNavigate() ? null : this.worldObj.getPathEntityToEntity(this.theEntity, var1, this.pathSearchRange, this.canPassOpenWoodenDoors, this.canPassClosedWoodenDoors, this.avoidsWater, this.canSwim);
	}

	public boolean func_48667_a(EntityLiving var1, float var2) {
		PathEntity var3 = this.func_48679_a(var1);
		return var3 != null ? this.setPath(var3, var2) : false;
	}

	public boolean setPath(PathEntity var1, float var2) {
		if(var1 == null) {
			this.currentPath = null;
			return false;
		} else {
			if(!var1.func_48647_a(this.currentPath)) {
				this.currentPath = var1;
			}

			if(this.noSunPathfind) {
				this.removeSunnyPath();
			}

			if(this.currentPath.getCurrentPathLength() == 0) {
				return false;
			} else {
				this.speed = var2;
				Vec3D var3 = this.getEntityPosition();
				this.ticksAtLastPos = this.totalTicks;
				this.lastPosCheck.xCoord = var3.xCoord;
				this.lastPosCheck.yCoord = var3.yCoord;
				this.lastPosCheck.zCoord = var3.zCoord;
				return true;
			}
		}
	}

	public PathEntity getPath() {
		return this.currentPath;
	}

	public void onUpdateNavigation() {
		++this.totalTicks;
		if(!this.noPath()) {
			if(this.canNavigate()) {
				this.pathFollow();
			}

			if(!this.noPath()) {
				Vec3D var1 = this.currentPath.getCurrentNodeVec3d(this.theEntity);
				if(var1 != null) {
					this.theEntity.getMoveHelper().setMoveTo(var1.xCoord, var1.yCoord, var1.zCoord, this.speed);
				}
			}
		}
	}

	private void pathFollow() {
		Vec3D var1 = this.getEntityPosition();
		int var2 = this.currentPath.getCurrentPathLength();

		for(int var3 = this.currentPath.getCurrentPathIndex(); var3 < this.currentPath.getCurrentPathLength(); ++var3) {
			if(this.currentPath.getPathPointFromIndex(var3).yCoord != (int)var1.yCoord) {
				var2 = var3;
				break;
			}
		}

		float var8 = this.theEntity.width * this.theEntity.width;

		int var4;
		for(var4 = this.currentPath.getCurrentPathIndex(); var4 < var2; ++var4) {
			if(var1.squareDistanceTo(this.currentPath.getVectorFromIndex(this.theEntity, var4)) < (double)var8) {
				this.currentPath.setCurrentPathIndex(var4 + 1);
			}
		}

		var4 = (int)Math.ceil((double)this.theEntity.width);
		int var5 = (int)this.theEntity.height + 1;
		int var6 = var4;

		for(int var7 = var2 - 1; var7 >= this.currentPath.getCurrentPathIndex(); --var7) {
			if(this.isDirectPathBetweenPoints(var1, this.currentPath.getVectorFromIndex(this.theEntity, var7), var4, var5, var6)) {
				this.currentPath.setCurrentPathIndex(var7);
				break;
			}
		}

		if(this.totalTicks - this.ticksAtLastPos > 100) {
			if(var1.squareDistanceTo(this.lastPosCheck) < 2.25D) {
				this.clearPathEntity();
			}

			this.ticksAtLastPos = this.totalTicks;
			this.lastPosCheck.xCoord = var1.xCoord;
			this.lastPosCheck.yCoord = var1.yCoord;
			this.lastPosCheck.zCoord = var1.zCoord;
		}

	}

	public boolean noPath() {
		return this.currentPath == null || this.currentPath.isFinished();
	}

	public void clearPathEntity() {
		this.currentPath = null;
	}

	private Vec3D getEntityPosition() {
		return Vec3D.createVector(this.theEntity.posX, (double)this.getPathableYPos(), this.theEntity.posZ);
	}

	private int getPathableYPos() {
		if(this.theEntity.isInWater() && this.canSwim) {
			int var1 = (int)this.theEntity.boundingBox.minY;
			int var2 = this.worldObj.getBlockId(MathHelper.floor_double(this.theEntity.posX), var1, MathHelper.floor_double(this.theEntity.posZ));
			int var3 = 0;

			do {
				if(var2 != Block.waterMoving.blockID && var2 != Block.waterStill.blockID) {
					return var1;
				}

				++var1;
				var2 = this.worldObj.getBlockId(MathHelper.floor_double(this.theEntity.posX), var1, MathHelper.floor_double(this.theEntity.posZ));
				++var3;
			} while(var3 <= 16);

			return (int)this.theEntity.boundingBox.minY;
		} else {
			return (int)(this.theEntity.boundingBox.minY + 0.5D);
		}
	}

	private boolean canNavigate() {
		return this.theEntity.onGround || this.canSwim && this.func_48657_k();
	}

	private boolean func_48657_k() {
		return this.theEntity.isInWater() || this.theEntity.handleLavaMovement();
	}

	private void removeSunnyPath() {
		if(!this.worldObj.canBlockSeeTheSky(MathHelper.floor_double(this.theEntity.posX), (int)(this.theEntity.boundingBox.minY + 0.5D), MathHelper.floor_double(this.theEntity.posZ))) {
			for(int var1 = 0; var1 < this.currentPath.getCurrentPathLength(); ++var1) {
				PathPoint var2 = this.currentPath.getPathPointFromIndex(var1);
				if(this.worldObj.canBlockSeeTheSky(var2.xCoord, var2.yCoord, var2.zCoord)) {
					this.currentPath.setCurrentPathLength(var1 - 1);
					return;
				}
			}

		}
	}

	private boolean isDirectPathBetweenPoints(Vec3D var1, Vec3D var2, int var3, int var4, int var5) {
		int var6 = MathHelper.floor_double(var1.xCoord);
		int var7 = MathHelper.floor_double(var1.zCoord);
		double var8 = var2.xCoord - var1.xCoord;
		double var10 = var2.zCoord - var1.zCoord;
		double var12 = var8 * var8 + var10 * var10;
		if(var12 < 1.0E-8D) {
			return false;
		} else {
			double var14 = 1.0D / Math.sqrt(var12);
			var8 *= var14;
			var10 *= var14;
			var3 += 2;
			var5 += 2;
			if(!this.isSafeToStandAt(var6, (int)var1.yCoord, var7, var3, var4, var5, var1, var8, var10)) {
				return false;
			} else {
				var3 -= 2;
				var5 -= 2;
				double var16 = 1.0D / Math.abs(var8);
				double var18 = 1.0D / Math.abs(var10);
				double var20 = (double)(var6 * 1) - var1.xCoord;
				double var22 = (double)(var7 * 1) - var1.zCoord;
				if(var8 >= 0.0D) {
					++var20;
				}

				if(var10 >= 0.0D) {
					++var22;
				}

				var20 /= var8;
				var22 /= var10;
				int var24 = var8 < 0.0D ? -1 : 1;
				int var25 = var10 < 0.0D ? -1 : 1;
				int var26 = MathHelper.floor_double(var2.xCoord);
				int var27 = MathHelper.floor_double(var2.zCoord);
				int var28 = var26 - var6;
				int var29 = var27 - var7;

				do {
					if(var28 * var24 <= 0 && var29 * var25 <= 0) {
						return true;
					}

					if(var20 < var22) {
						var20 += var16;
						var6 += var24;
						var28 = var26 - var6;
					} else {
						var22 += var18;
						var7 += var25;
						var29 = var27 - var7;
					}
				} while(this.isSafeToStandAt(var6, (int)var1.yCoord, var7, var3, var4, var5, var1, var8, var10));

				return false;
			}
		}
	}

	private boolean isSafeToStandAt(int var1, int var2, int var3, int var4, int var5, int var6, Vec3D var7, double var8, double var10) {
		int var12 = var1 - var4 / 2;
		int var13 = var3 - var6 / 2;
		if(!this.isPositionClear(var12, var2, var13, var4, var5, var6, var7, var8, var10)) {
			return false;
		} else {
			for(int var14 = var12; var14 < var12 + var4; ++var14) {
				for(int var15 = var13; var15 < var13 + var6; ++var15) {
					double var16 = (double)var14 + 0.5D - var7.xCoord;
					double var18 = (double)var15 + 0.5D - var7.zCoord;
					if(var16 * var8 + var18 * var10 >= 0.0D) {
						int var20 = this.worldObj.getBlockId(var14, var2 - 1, var15);
						if(var20 <= 0) {
							return false;
						}

						Material var21 = Block.blocksList[var20].blockMaterial;
						if(var21 == Material.water && !this.theEntity.isInWater()) {
							return false;
						}

						if(var21 == Material.lava) {
							return false;
						}
					}
				}
			}

			return true;
		}
	}

	private boolean isPositionClear(int var1, int var2, int var3, int var4, int var5, int var6, Vec3D var7, double var8, double var10) {
		for(int var12 = var1; var12 < var1 + var4; ++var12) {
			for(int var13 = var2; var13 < var2 + var5; ++var13) {
				for(int var14 = var3; var14 < var3 + var6; ++var14) {
					double var15 = (double)var12 + 0.5D - var7.xCoord;
					double var17 = (double)var14 + 0.5D - var7.zCoord;
					if(var15 * var8 + var17 * var10 >= 0.0D) {
						int var19 = this.worldObj.getBlockId(var12, var13, var14);
						if(var19 > 0 && !Block.blocksList[var19].getBlocksMovement(this.worldObj, var12, var13, var14)) {
							return false;
						}
					}
				}
			}
		}

		return true;
	}
}
