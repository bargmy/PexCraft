package net.minecraft.src;

public class VillageDoorInfo {
	public final int posX;
	public final int posY;
	public final int posZ;
	public final int insideDirectionX;
	public final int insideDirectionZ;
	public int lastActivityTimestamp;
	public boolean isDetachedFromVillageFlag = false;
	private int doorOpeningRestrictionCounter = 0;

	public VillageDoorInfo(int var1, int var2, int var3, int var4, int var5, int var6) {
		this.posX = var1;
		this.posY = var2;
		this.posZ = var3;
		this.insideDirectionX = var4;
		this.insideDirectionZ = var5;
		this.lastActivityTimestamp = var6;
	}

	public int getDistanceSquared(int var1, int var2, int var3) {
		int var4 = var1 - this.posX;
		int var5 = var2 - this.posY;
		int var6 = var3 - this.posZ;
		return var4 * var4 + var5 * var5 + var6 * var6;
	}

	public int getInsideDistanceSquare(int var1, int var2, int var3) {
		int var4 = var1 - this.posX - this.insideDirectionX;
		int var5 = var2 - this.posY;
		int var6 = var3 - this.posZ - this.insideDirectionZ;
		return var4 * var4 + var5 * var5 + var6 * var6;
	}

	public int getInsidePosX() {
		return this.posX + this.insideDirectionX;
	}

	public int getInsidePosY() {
		return this.posY;
	}

	public int getInsidePosZ() {
		return this.posZ + this.insideDirectionZ;
	}

	public boolean isInside(int var1, int var2) {
		int var3 = var1 - this.posX;
		int var4 = var2 - this.posZ;
		return var3 * this.insideDirectionX + var4 * this.insideDirectionZ >= 0;
	}

	public void resetDoorOpeningRestrictionCounter() {
		this.doorOpeningRestrictionCounter = 0;
	}

	public void incrementDoorOpeningRestrictionCounter() {
		++this.doorOpeningRestrictionCounter;
	}

	public int getDoorOpeningRestrictionCounter() {
		return this.doorOpeningRestrictionCounter;
	}
}
