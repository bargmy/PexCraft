package net.minecraft.src;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class Village {
	private final World worldObj;
	private final List villageDoorInfoList = new ArrayList();
	private final ChunkCoordinates centerHelper = new ChunkCoordinates(0, 0, 0);
	private final ChunkCoordinates center = new ChunkCoordinates(0, 0, 0);
	private int villageRadius = 0;
	private int lastAddDoorTimestamp = 0;
	private int tickCounter = 0;
	private int numVillagers = 0;
	private List villageAgressors = new ArrayList();
	private int numIronGolems = 0;

	public Village(World var1) {
		this.worldObj = var1;
	}

	public void tick(int var1) {
		this.tickCounter = var1;
		this.removeDeadAndOutOfRangeDoors();
		this.removeDeadAndOldAgressors();
		if(var1 % 20 == 0) {
			this.updateNumVillagers();
		}

		if(var1 % 30 == 0) {
			this.updateNumIronGolems();
		}

		int var2 = this.numVillagers / 16;
		if(this.numIronGolems < var2 && this.villageDoorInfoList.size() > 20 && this.worldObj.rand.nextInt(7000) == 0) {
			Vec3D var3 = this.tryGetIronGolemSpawningLocation(MathHelper.floor_float((float)this.center.posX), MathHelper.floor_float((float)this.center.posY), MathHelper.floor_float((float)this.center.posZ), 2, 4, 2);
			if(var3 != null) {
				EntityIronGolem var4 = new EntityIronGolem(this.worldObj);
				var4.setPosition(var3.xCoord, var3.yCoord, var3.zCoord);
				this.worldObj.spawnEntityInWorld(var4);
				++this.numIronGolems;
			}
		}

	}

	private Vec3D tryGetIronGolemSpawningLocation(int var1, int var2, int var3, int var4, int var5, int var6) {
		for(int var7 = 0; var7 < 10; ++var7) {
			int var8 = var1 + this.worldObj.rand.nextInt(16) - 8;
			int var9 = var2 + this.worldObj.rand.nextInt(6) - 3;
			int var10 = var3 + this.worldObj.rand.nextInt(16) - 8;
			if(this.isInRange(var8, var9, var10) && this.isValidIronGolemSpawningLocation(var8, var9, var10, var4, var5, var6)) {
				return Vec3D.createVector((double)var8, (double)var9, (double)var10);
			}
		}

		return null;
	}

	private boolean isValidIronGolemSpawningLocation(int var1, int var2, int var3, int var4, int var5, int var6) {
		if(!this.worldObj.isBlockNormalCube(var1, var2 - 1, var3)) {
			return false;
		} else {
			int var7 = var1 - var4 / 2;
			int var8 = var3 - var6 / 2;

			for(int var9 = var7; var9 < var7 + var4; ++var9) {
				for(int var10 = var2; var10 < var2 + var5; ++var10) {
					for(int var11 = var8; var11 < var8 + var6; ++var11) {
						if(this.worldObj.isBlockNormalCube(var9, var10, var11)) {
							return false;
						}
					}
				}
			}

			return true;
		}
	}

	private void updateNumIronGolems() {
		List var1 = this.worldObj.getEntitiesWithinAABB(EntityIronGolem.class, AxisAlignedBB.getBoundingBoxFromPool((double)(this.center.posX - this.villageRadius), (double)(this.center.posY - 4), (double)(this.center.posZ - this.villageRadius), (double)(this.center.posX + this.villageRadius), (double)(this.center.posY + 4), (double)(this.center.posZ + this.villageRadius)));
		this.numIronGolems = var1.size();
	}

	private void updateNumVillagers() {
		List var1 = this.worldObj.getEntitiesWithinAABB(EntityVillager.class, AxisAlignedBB.getBoundingBoxFromPool((double)(this.center.posX - this.villageRadius), (double)(this.center.posY - 4), (double)(this.center.posZ - this.villageRadius), (double)(this.center.posX + this.villageRadius), (double)(this.center.posY + 4), (double)(this.center.posZ + this.villageRadius)));
		this.numVillagers = var1.size();
	}

	public ChunkCoordinates getCenter() {
		return this.center;
	}

	public int getVillageRadius() {
		return this.villageRadius;
	}

	public int getNumVillageDoors() {
		return this.villageDoorInfoList.size();
	}

	public int getTicksSinceLastDoorAdding() {
		return this.tickCounter - this.lastAddDoorTimestamp;
	}

	public int getNumVillagers() {
		return this.numVillagers;
	}

	public boolean isInRange(int var1, int var2, int var3) {
		return this.center.getDistanceSquared(var1, var2, var3) < (float)(this.villageRadius * this.villageRadius);
	}

	public List getVillageDoorInfoList() {
		return this.villageDoorInfoList;
	}

	public VillageDoorInfo findNearestDoor(int var1, int var2, int var3) {
		VillageDoorInfo var4 = null;
		int var5 = Integer.MAX_VALUE;
		Iterator var6 = this.villageDoorInfoList.iterator();

		while(var6.hasNext()) {
			VillageDoorInfo var7 = (VillageDoorInfo)var6.next();
			int var8 = var7.getDistanceSquared(var1, var2, var3);
			if(var8 < var5) {
				var4 = var7;
				var5 = var8;
			}
		}

		return var4;
	}

	public VillageDoorInfo findNearestDoorUnrestricted(int var1, int var2, int var3) {
		VillageDoorInfo var4 = null;
		int var5 = Integer.MAX_VALUE;
		Iterator var6 = this.villageDoorInfoList.iterator();

		while(var6.hasNext()) {
			VillageDoorInfo var7 = (VillageDoorInfo)var6.next();
			int var8 = var7.getDistanceSquared(var1, var2, var3);
			if(var8 > 256) {
				var8 *= 1000;
			} else {
				var8 = var7.getDoorOpeningRestrictionCounter();
			}

			if(var8 < var5) {
				var4 = var7;
				var5 = var8;
			}
		}

		return var4;
	}

	public VillageDoorInfo getVillageDoorAt(int var1, int var2, int var3) {
		if(this.center.getDistanceSquared(var1, var2, var3) > (float)(this.villageRadius * this.villageRadius)) {
			return null;
		} else {
			Iterator var4 = this.villageDoorInfoList.iterator();

			VillageDoorInfo var5;
			do {
				if(!var4.hasNext()) {
					return null;
				}

				var5 = (VillageDoorInfo)var4.next();
			} while(var5.posX != var1 || var5.posZ != var3 || Math.abs(var5.posY - var2) > 1);

			return var5;
		}
	}

	public void addVillageDoorInfo(VillageDoorInfo var1) {
		this.villageDoorInfoList.add(var1);
		this.centerHelper.posX += var1.posX;
		this.centerHelper.posY += var1.posY;
		this.centerHelper.posZ += var1.posZ;
		this.updateVillageRadiusAndCenter();
		this.lastAddDoorTimestamp = var1.lastActivityTimestamp;
	}

	public boolean isAnnihilated() {
		return this.villageDoorInfoList.isEmpty();
	}

	public void addOrRenewAgressor(EntityLiving var1) {
		Iterator var2 = this.villageAgressors.iterator();

		VillageAgressor var3;
		do {
			if(!var2.hasNext()) {
				this.villageAgressors.add(new VillageAgressor(this, var1, this.tickCounter));
				return;
			}

			var3 = (VillageAgressor)var2.next();
		} while(var3.agressor != var1);

		var3.agressionTime = this.tickCounter;
	}

	public EntityLiving findNearestVillageAggressor(EntityLiving var1) {
		double var2 = Double.MAX_VALUE;
		VillageAgressor var4 = null;

		for(int var5 = 0; var5 < this.villageAgressors.size(); ++var5) {
			VillageAgressor var6 = (VillageAgressor)this.villageAgressors.get(var5);
			double var7 = var6.agressor.getDistanceSqToEntity(var1);
			if(var7 <= var2) {
				var4 = var6;
				var2 = var7;
			}
		}

		return var4 != null ? var4.agressor : null;
	}

	private void removeDeadAndOldAgressors() {
		Iterator var1 = this.villageAgressors.iterator();

		while(true) {
			VillageAgressor var2;
			do {
				if(!var1.hasNext()) {
					return;
				}

				var2 = (VillageAgressor)var1.next();
			} while(var2.agressor.isEntityAlive() && Math.abs(this.tickCounter - var2.agressionTime) <= 300);

			var1.remove();
		}
	}

	private void removeDeadAndOutOfRangeDoors() {
		boolean var1 = false;
		boolean var2 = this.worldObj.rand.nextInt(50) == 0;
		Iterator var3 = this.villageDoorInfoList.iterator();

		while(true) {
			VillageDoorInfo var4;
			do {
				if(!var3.hasNext()) {
					if(var1) {
						this.updateVillageRadiusAndCenter();
					}

					return;
				}

				var4 = (VillageDoorInfo)var3.next();
				if(var2) {
					var4.resetDoorOpeningRestrictionCounter();
				}
			} while(this.isBlockDoor(var4.posX, var4.posY, var4.posZ) && Math.abs(this.tickCounter - var4.lastActivityTimestamp) <= 1200);

			this.centerHelper.posX -= var4.posX;
			this.centerHelper.posY -= var4.posY;
			this.centerHelper.posZ -= var4.posZ;
			var1 = true;
			var4.isDetachedFromVillageFlag = true;
			var3.remove();
		}
	}

	private boolean isBlockDoor(int var1, int var2, int var3) {
		int var4 = this.worldObj.getBlockId(var1, var2, var3);
		return var4 <= 0 ? false : var4 == Block.doorWood.blockID;
	}

	private void updateVillageRadiusAndCenter() {
		int var1 = this.villageDoorInfoList.size();
		if(var1 == 0) {
			this.center.set(0, 0, 0);
			this.villageRadius = 0;
		} else {
			this.center.set(this.centerHelper.posX / var1, this.centerHelper.posY / var1, this.centerHelper.posZ / var1);
			int var2 = 0;

			VillageDoorInfo var4;
			for(Iterator var3 = this.villageDoorInfoList.iterator(); var3.hasNext(); var2 = Math.max(var4.getDistanceSquared(this.center.posX, this.center.posY, this.center.posZ), var2)) {
				var4 = (VillageDoorInfo)var3.next();
			}

			this.villageRadius = Math.max(32, (int)Math.sqrt((double)var2) + 1);
		}
	}
}
