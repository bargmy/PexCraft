package net.minecraft.src;

public class WorldProviderEnd extends WorldProvider {
	public void registerWorldChunkManager() {
		this.worldChunkMgr = new WorldChunkManagerHell(BiomeGenBase.sky, 0.5F, 0.0F);
		this.worldType = 1;
		this.hasNoSky = true;
	}

	public IChunkProvider getChunkProvider() {
		return new ChunkProviderEnd(this.worldObj, this.worldObj.getSeed());
	}

	public float calculateCelestialAngle(long var1, float var3) {
		return 0.0F;
	}

	public float[] calcSunriseSunsetColors(float var1, float var2) {
		return null;
	}

	public Vec3D getFogColor(float var1, float var2) {
		int var3 = 8421536;
		float var4 = MathHelper.cos(var1 * (float)Math.PI * 2.0F) * 2.0F + 0.5F;
		if(var4 < 0.0F) {
			var4 = 0.0F;
		}

		if(var4 > 1.0F) {
			var4 = 1.0F;
		}

		float var5 = (float)(var3 >> 16 & 255) / 255.0F;
		float var6 = (float)(var3 >> 8 & 255) / 255.0F;
		float var7 = (float)(var3 & 255) / 255.0F;
		var5 *= var4 * 0.0F + 0.15F;
		var6 *= var4 * 0.0F + 0.15F;
		var7 *= var4 * 0.0F + 0.15F;
		return Vec3D.createVector((double)var5, (double)var6, (double)var7);
	}

	public boolean isSkyColored() {
		return false;
	}

	public boolean canRespawnHere() {
		return false;
	}

	public boolean func_48217_e() {
		return false;
	}

	public float getCloudHeight() {
		return 8.0F;
	}

	public boolean canCoordinateBeSpawn(int var1, int var2) {
		int var3 = this.worldObj.getFirstUncoveredBlock(var1, var2);
		return var3 == 0 ? false : Block.blocksList[var3].blockMaterial.blocksMovement();
	}

	public ChunkCoordinates getEntrancePortalLocation() {
		return new ChunkCoordinates(100, 50, 0);
	}

	public int getAverageGroundLevel() {
		return 50;
	}

	public boolean func_48218_b(int var1, int var2) {
		return true;
	}
}
