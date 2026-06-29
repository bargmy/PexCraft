package net.minecraft.src;

public class PathFinder {
	private IBlockAccess worldMap;
	private Path path = new Path();
	private IntHashMap pointMap = new IntHashMap();
	private PathPoint[] pathOptions = new PathPoint[32];
	private boolean isWoddenDoorAllowed;
	private boolean isMovementBlockAllowed;
	private boolean isPathingInWater;
	private boolean canEntityDrown;

	public PathFinder(IBlockAccess var1, boolean var2, boolean var3, boolean var4, boolean var5) {
		this.worldMap = var1;
		this.isWoddenDoorAllowed = var2;
		this.isMovementBlockAllowed = var3;
		this.isPathingInWater = var4;
		this.canEntityDrown = var5;
	}

	public PathEntity createEntityPathTo(Entity var1, Entity var2, float var3) {
		return this.createEntityPathTo(var1, var2.posX, var2.boundingBox.minY, var2.posZ, var3);
	}

	public PathEntity createEntityPathTo(Entity var1, int var2, int var3, int var4, float var5) {
		return this.createEntityPathTo(var1, (double)((float)var2 + 0.5F), (double)((float)var3 + 0.5F), (double)((float)var4 + 0.5F), var5);
	}

	private PathEntity createEntityPathTo(Entity var1, double var2, double var4, double var6, float var8) {
		this.path.clearPath();
		this.pointMap.clearMap();
		boolean var9 = this.isPathingInWater;
		int var10 = MathHelper.floor_double(var1.boundingBox.minY + 0.5D);
		if(this.canEntityDrown && var1.isInWater()) {
			var10 = (int)var1.boundingBox.minY;

			for(int var11 = this.worldMap.getBlockId(MathHelper.floor_double(var1.posX), var10, MathHelper.floor_double(var1.posZ)); var11 == Block.waterMoving.blockID || var11 == Block.waterStill.blockID; var11 = this.worldMap.getBlockId(MathHelper.floor_double(var1.posX), var10, MathHelper.floor_double(var1.posZ))) {
				++var10;
			}

			var9 = this.isPathingInWater;
			this.isPathingInWater = false;
		} else {
			var10 = MathHelper.floor_double(var1.boundingBox.minY + 0.5D);
		}

		PathPoint var15 = this.openPoint(MathHelper.floor_double(var1.boundingBox.minX), var10, MathHelper.floor_double(var1.boundingBox.minZ));
		PathPoint var12 = this.openPoint(MathHelper.floor_double(var2 - (double)(var1.width / 2.0F)), MathHelper.floor_double(var4), MathHelper.floor_double(var6 - (double)(var1.width / 2.0F)));
		PathPoint var13 = new PathPoint(MathHelper.floor_float(var1.width + 1.0F), MathHelper.floor_float(var1.height + 1.0F), MathHelper.floor_float(var1.width + 1.0F));
		PathEntity var14 = this.addToPath(var1, var15, var12, var13, var8);
		this.isPathingInWater = var9;
		return var14;
	}

	private PathEntity addToPath(Entity var1, PathPoint var2, PathPoint var3, PathPoint var4, float var5) {
		var2.totalPathDistance = 0.0F;
		var2.distanceToNext = var2.distanceTo(var3);
		var2.distanceToTarget = var2.distanceToNext;
		this.path.clearPath();
		this.path.addPoint(var2);
		PathPoint var6 = var2;

		while(!this.path.isPathEmpty()) {
			PathPoint var7 = this.path.dequeue();
			if(var7.equals(var3)) {
				return this.createEntityPath(var2, var3);
			}

			if(var7.distanceTo(var3) < var6.distanceTo(var3)) {
				var6 = var7;
			}

			var7.isFirst = true;
			int var8 = this.findPathOptions(var1, var7, var4, var3, var5);

			for(int var9 = 0; var9 < var8; ++var9) {
				PathPoint var10 = this.pathOptions[var9];
				float var11 = var7.totalPathDistance + var7.distanceTo(var10);
				if(!var10.isAssigned() || var11 < var10.totalPathDistance) {
					var10.previous = var7;
					var10.totalPathDistance = var11;
					var10.distanceToNext = var10.distanceTo(var3);
					if(var10.isAssigned()) {
						this.path.changeDistance(var10, var10.totalPathDistance + var10.distanceToNext);
					} else {
						var10.distanceToTarget = var10.totalPathDistance + var10.distanceToNext;
						this.path.addPoint(var10);
					}
				}
			}
		}

		if(var6 == var2) {
			return null;
		} else {
			return this.createEntityPath(var2, var6);
		}
	}

	private int findPathOptions(Entity var1, PathPoint var2, PathPoint var3, PathPoint var4, float var5) {
		int var6 = 0;
		byte var7 = 0;
		if(this.getVerticalOffset(var1, var2.xCoord, var2.yCoord + 1, var2.zCoord, var3) == 1) {
			var7 = 1;
		}

		PathPoint var8 = this.getSafePoint(var1, var2.xCoord, var2.yCoord, var2.zCoord + 1, var3, var7);
		PathPoint var9 = this.getSafePoint(var1, var2.xCoord - 1, var2.yCoord, var2.zCoord, var3, var7);
		PathPoint var10 = this.getSafePoint(var1, var2.xCoord + 1, var2.yCoord, var2.zCoord, var3, var7);
		PathPoint var11 = this.getSafePoint(var1, var2.xCoord, var2.yCoord, var2.zCoord - 1, var3, var7);
		if(var8 != null && !var8.isFirst && var8.distanceTo(var4) < var5) {
			this.pathOptions[var6++] = var8;
		}

		if(var9 != null && !var9.isFirst && var9.distanceTo(var4) < var5) {
			this.pathOptions[var6++] = var9;
		}

		if(var10 != null && !var10.isFirst && var10.distanceTo(var4) < var5) {
			this.pathOptions[var6++] = var10;
		}

		if(var11 != null && !var11.isFirst && var11.distanceTo(var4) < var5) {
			this.pathOptions[var6++] = var11;
		}

		return var6;
	}

	private PathPoint getSafePoint(Entity var1, int var2, int var3, int var4, PathPoint var5, int var6) {
		PathPoint var7 = null;
		int var8 = this.getVerticalOffset(var1, var2, var3, var4, var5);
		if(var8 == 2) {
			return this.openPoint(var2, var3, var4);
		} else {
			if(var8 == 1) {
				var7 = this.openPoint(var2, var3, var4);
			}

			if(var7 == null && var6 > 0 && var8 != -3 && var8 != -4 && this.getVerticalOffset(var1, var2, var3 + var6, var4, var5) == 1) {
				var7 = this.openPoint(var2, var3 + var6, var4);
				var3 += var6;
			}

			if(var7 != null) {
				int var9 = 0;
				int var10 = 0;

				while(var3 > 0) {
					var10 = this.getVerticalOffset(var1, var2, var3 - 1, var4, var5);
					if(this.isPathingInWater && var10 == -1) {
						return null;
					}

					if(var10 != 1) {
						break;
					}

					++var9;
					if(var9 >= 4) {
						return null;
					}

					--var3;
					if(var3 > 0) {
						var7 = this.openPoint(var2, var3, var4);
					}
				}

				if(var10 == -2) {
					return null;
				}
			}

			return var7;
		}
	}

	private final PathPoint openPoint(int var1, int var2, int var3) {
		int var4 = PathPoint.makeHash(var1, var2, var3);
		PathPoint var5 = (PathPoint)this.pointMap.lookup(var4);
		if(var5 == null) {
			var5 = new PathPoint(var1, var2, var3);
			this.pointMap.addKey(var4, var5);
		}

		return var5;
	}

	private int getVerticalOffset(Entity var1, int var2, int var3, int var4, PathPoint var5) {
		boolean var6 = false;

		for(int var7 = var2; var7 < var2 + var5.xCoord; ++var7) {
			for(int var8 = var3; var8 < var3 + var5.yCoord; ++var8) {
				for(int var9 = var4; var9 < var4 + var5.zCoord; ++var9) {
					int var10 = this.worldMap.getBlockId(var7, var8, var9);
					if(var10 > 0) {
						if(var10 == Block.trapdoor.blockID) {
							var6 = true;
						} else if(var10 != Block.waterMoving.blockID && var10 != Block.waterStill.blockID) {
							if(!this.isWoddenDoorAllowed && var10 == Block.doorWood.blockID) {
								return 0;
							}
						} else {
							if(this.isPathingInWater) {
								return -1;
							}

							var6 = true;
						}

						Block var11 = Block.blocksList[var10];
						if(!var11.getBlocksMovement(this.worldMap, var7, var8, var9) && (!this.isMovementBlockAllowed || var10 != Block.doorWood.blockID)) {
							if(var10 == Block.fence.blockID || var10 == Block.fenceGate.blockID) {
								return -3;
							}

							if(var10 == Block.trapdoor.blockID) {
								return -4;
							}

							Material var12 = var11.blockMaterial;
							if(var12 != Material.lava) {
								return 0;
							}

							if(!var1.handleLavaMovement()) {
								return -2;
							}
						}
					}
				}
			}
		}

		return var6 ? 2 : 1;
	}

	private PathEntity createEntityPath(PathPoint var1, PathPoint var2) {
		int var3 = 1;

		PathPoint var4;
		for(var4 = var2; var4.previous != null; var4 = var4.previous) {
			++var3;
		}

		PathPoint[] var5 = new PathPoint[var3];
		var4 = var2;
		--var3;

		for(var5[var3] = var2; var4.previous != null; var5[var3] = var4) {
			var4 = var4.previous;
			--var3;
		}

		return new PathEntity(var5);
	}
}
