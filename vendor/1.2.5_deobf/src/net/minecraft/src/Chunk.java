package net.minecraft.src;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class Chunk {
	public static boolean isLit;
	private ExtendedBlockStorage[] storageArrays;
	private byte[] blockBiomeArray;
	public int[] precipitationHeightMap;
	public boolean[] updateSkylightColumns;
	public boolean isChunkLoaded;
	public World worldObj;
	public int[] heightMap;
	public final int xPosition;
	public final int zPosition;
	private boolean isGapLightingUpdated;
	public Map chunkTileEntityMap;
	public List[] entityLists;
	public boolean isTerrainPopulated;
	public boolean isModified;
	public boolean hasEntities;
	public long lastSaveTime;
	public boolean field_50120_o;
	private int queuedLightChecks;
	boolean field_35846_u;

	public Chunk(World var1, int var2, int var3) {
		this.storageArrays = new ExtendedBlockStorage[16];
		this.blockBiomeArray = new byte[256];
		this.precipitationHeightMap = new int[256];
		this.updateSkylightColumns = new boolean[256];
		this.isGapLightingUpdated = false;
		this.chunkTileEntityMap = new HashMap();
		this.isTerrainPopulated = false;
		this.isModified = false;
		this.hasEntities = false;
		this.lastSaveTime = 0L;
		this.field_50120_o = false;
		this.queuedLightChecks = 4096;
		this.field_35846_u = false;
		this.entityLists = new List[16];
		this.worldObj = var1;
		this.xPosition = var2;
		this.zPosition = var3;
		this.heightMap = new int[256];

		for(int var4 = 0; var4 < this.entityLists.length; ++var4) {
			this.entityLists[var4] = new ArrayList();
		}

		Arrays.fill(this.precipitationHeightMap, -999);
		Arrays.fill(this.blockBiomeArray, (byte)-1);
	}

	public Chunk(World var1, byte[] var2, int var3, int var4) {
		this(var1, var3, var4);
		int var5 = var2.length / 256;

		for(int var6 = 0; var6 < 16; ++var6) {
			for(int var7 = 0; var7 < 16; ++var7) {
				for(int var8 = 0; var8 < var5; ++var8) {
					byte var9 = var2[var6 << 11 | var7 << 7 | var8];
					if(var9 != 0) {
						int var10 = var8 >> 4;
						if(this.storageArrays[var10] == null) {
							this.storageArrays[var10] = new ExtendedBlockStorage(var10 << 4);
						}

						this.storageArrays[var10].setExtBlockID(var6, var8 & 15, var7, var9);
					}
				}
			}
		}

	}

	public boolean isAtLocation(int var1, int var2) {
		return var1 == this.xPosition && var2 == this.zPosition;
	}

	public int getHeightValue(int var1, int var2) {
		return this.heightMap[var2 << 4 | var1];
	}

	public int getTopFilledSegment() {
		for(int var1 = this.storageArrays.length - 1; var1 >= 0; --var1) {
			if(this.storageArrays[var1] != null) {
				return this.storageArrays[var1].getYLocation();
			}
		}

		return 0;
	}

	public ExtendedBlockStorage[] getBlockStorageArray() {
		return this.storageArrays;
	}

	public void generateHeightMap() {
		int var1 = this.getTopFilledSegment();

		for(int var2 = 0; var2 < 16; ++var2) {
			for(int var3 = 0; var3 < 16; ++var3) {
				this.precipitationHeightMap[var2 + (var3 << 4)] = -999;

				for(int var4 = var1 + 16 - 1; var4 > 0; --var4) {
					int var5 = this.getBlockID(var2, var4 - 1, var3);
					if(Block.lightOpacity[var5] != 0) {
						this.heightMap[var3 << 4 | var2] = var4;
						break;
					}
				}
			}
		}

		this.isModified = true;
	}

	public void generateSkylightMap() {
		int var1 = this.getTopFilledSegment();

		int var2;
		int var3;
		for(var2 = 0; var2 < 16; ++var2) {
			for(var3 = 0; var3 < 16; ++var3) {
				this.precipitationHeightMap[var2 + (var3 << 4)] = -999;

				int var4;
				for(var4 = var1 + 16 - 1; var4 > 0; --var4) {
					if(this.getBlockLightOpacity(var2, var4 - 1, var3) != 0) {
						this.heightMap[var3 << 4 | var2] = var4;
						break;
					}
				}

				if(!this.worldObj.worldProvider.hasNoSky) {
					var4 = 15;
					int var5 = var1 + 16 - 1;

					do {
						var4 -= this.getBlockLightOpacity(var2, var5, var3);
						if(var4 > 0) {
							ExtendedBlockStorage var6 = this.storageArrays[var5 >> 4];
							if(var6 != null) {
								var6.setExtSkylightValue(var2, var5 & 15, var3, var4);
								this.worldObj.func_48464_p((this.xPosition << 4) + var2, var5, (this.zPosition << 4) + var3);
							}
						}

						--var5;
					} while(var5 > 0 && var4 > 0);
				}
			}
		}

		this.isModified = true;

		for(var2 = 0; var2 < 16; ++var2) {
			for(var3 = 0; var3 < 16; ++var3) {
				this.propagateSkylightOcclusion(var2, var3);
			}
		}

	}

	public void func_4143_d() {
	}

	private void propagateSkylightOcclusion(int var1, int var2) {
		this.updateSkylightColumns[var1 + var2 * 16] = true;
		this.isGapLightingUpdated = true;
	}

	private void updateSkylight_do() {
		Profiler.startSection("recheckGaps");
		if(this.worldObj.doChunksNearChunkExist(this.xPosition * 16 + 8, 0, this.zPosition * 16 + 8, 16)) {
			for(int var1 = 0; var1 < 16; ++var1) {
				for(int var2 = 0; var2 < 16; ++var2) {
					if(this.updateSkylightColumns[var1 + var2 * 16]) {
						this.updateSkylightColumns[var1 + var2 * 16] = false;
						int var3 = this.getHeightValue(var1, var2);
						int var4 = this.xPosition * 16 + var1;
						int var5 = this.zPosition * 16 + var2;
						int var6 = this.worldObj.getHeightValue(var4 - 1, var5);
						int var7 = this.worldObj.getHeightValue(var4 + 1, var5);
						int var8 = this.worldObj.getHeightValue(var4, var5 - 1);
						int var9 = this.worldObj.getHeightValue(var4, var5 + 1);
						if(var7 < var6) {
							var6 = var7;
						}

						if(var8 < var6) {
							var6 = var8;
						}

						if(var9 < var6) {
							var6 = var9;
						}

						this.checkSkylightNeighborHeight(var4, var5, var6);
						this.checkSkylightNeighborHeight(var4 - 1, var5, var3);
						this.checkSkylightNeighborHeight(var4 + 1, var5, var3);
						this.checkSkylightNeighborHeight(var4, var5 - 1, var3);
						this.checkSkylightNeighborHeight(var4, var5 + 1, var3);
					}
				}
			}

			this.isGapLightingUpdated = false;
		}

		Profiler.endSection();
	}

	private void checkSkylightNeighborHeight(int var1, int var2, int var3) {
		int var4 = this.worldObj.getHeightValue(var1, var2);
		if(var4 > var3) {
			this.updateSkylightNeighborHeight(var1, var2, var3, var4 + 1);
		} else if(var4 < var3) {
			this.updateSkylightNeighborHeight(var1, var2, var4, var3 + 1);
		}

	}

	private void updateSkylightNeighborHeight(int var1, int var2, int var3, int var4) {
		if(var4 > var3 && this.worldObj.doChunksNearChunkExist(var1, 0, var2, 16)) {
			for(int var5 = var3; var5 < var4; ++var5) {
				this.worldObj.updateLightByType(EnumSkyBlock.Sky, var1, var5, var2);
			}

			this.isModified = true;
		}

	}

	private void relightBlock(int var1, int var2, int var3) {
		int var4 = this.heightMap[var3 << 4 | var1] & 255;
		int var5 = var4;
		if(var2 > var4) {
			var5 = var2;
		}

		while(var5 > 0 && this.getBlockLightOpacity(var1, var5 - 1, var3) == 0) {
			--var5;
		}

		if(var5 != var4) {
			this.worldObj.markBlocksDirtyVertical(var1, var3, var5, var4);
			this.heightMap[var3 << 4 | var1] = var5;
			int var6 = this.xPosition * 16 + var1;
			int var7 = this.zPosition * 16 + var3;
			int var8;
			int var12;
			if(!this.worldObj.worldProvider.hasNoSky) {
				ExtendedBlockStorage var9;
				if(var5 < var4) {
					for(var8 = var5; var8 < var4; ++var8) {
						var9 = this.storageArrays[var8 >> 4];
						if(var9 != null) {
							var9.setExtSkylightValue(var1, var8 & 15, var3, 15);
							this.worldObj.func_48464_p((this.xPosition << 4) + var1, var8, (this.zPosition << 4) + var3);
						}
					}
				} else {
					for(var8 = var4; var8 < var5; ++var8) {
						var9 = this.storageArrays[var8 >> 4];
						if(var9 != null) {
							var9.setExtSkylightValue(var1, var8 & 15, var3, 0);
							this.worldObj.func_48464_p((this.xPosition << 4) + var1, var8, (this.zPosition << 4) + var3);
						}
					}
				}

				var8 = 15;

				while(var5 > 0 && var8 > 0) {
					--var5;
					var12 = this.getBlockLightOpacity(var1, var5, var3);
					if(var12 == 0) {
						var12 = 1;
					}

					var8 -= var12;
					if(var8 < 0) {
						var8 = 0;
					}

					ExtendedBlockStorage var10 = this.storageArrays[var5 >> 4];
					if(var10 != null) {
						var10.setExtSkylightValue(var1, var5 & 15, var3, var8);
					}
				}
			}

			var8 = this.heightMap[var3 << 4 | var1];
			var12 = var4;
			int var13 = var8;
			if(var8 < var4) {
				var12 = var8;
				var13 = var4;
			}

			if(!this.worldObj.worldProvider.hasNoSky) {
				this.updateSkylightNeighborHeight(var6 - 1, var7, var12, var13);
				this.updateSkylightNeighborHeight(var6 + 1, var7, var12, var13);
				this.updateSkylightNeighborHeight(var6, var7 - 1, var12, var13);
				this.updateSkylightNeighborHeight(var6, var7 + 1, var12, var13);
				this.updateSkylightNeighborHeight(var6, var7, var12, var13);
			}

			this.isModified = true;
		}
	}

	public int getBlockLightOpacity(int var1, int var2, int var3) {
		return Block.lightOpacity[this.getBlockID(var1, var2, var3)];
	}

	public int getBlockID(int var1, int var2, int var3) {
		if(var2 >> 4 >= this.storageArrays.length) {
			return 0;
		} else {
			ExtendedBlockStorage var4 = this.storageArrays[var2 >> 4];
			return var4 != null ? var4.getExtBlockID(var1, var2 & 15, var3) : 0;
		}
	}

	public int getBlockMetadata(int var1, int var2, int var3) {
		if(var2 >> 4 >= this.storageArrays.length) {
			return 0;
		} else {
			ExtendedBlockStorage var4 = this.storageArrays[var2 >> 4];
			return var4 != null ? var4.getExtBlockMetadata(var1, var2 & 15, var3) : 0;
		}
	}

	public boolean setBlockID(int var1, int var2, int var3, int var4) {
		return this.setBlockIDWithMetadata(var1, var2, var3, var4, 0);
	}

	public boolean setBlockIDWithMetadata(int var1, int var2, int var3, int var4, int var5) {
		int var6 = var3 << 4 | var1;
		if(var2 >= this.precipitationHeightMap[var6] - 1) {
			this.precipitationHeightMap[var6] = -999;
		}

		int var7 = this.heightMap[var6];
		int var8 = this.getBlockID(var1, var2, var3);
		if(var8 == var4 && this.getBlockMetadata(var1, var2, var3) == var5) {
			return false;
		} else {
			ExtendedBlockStorage var9 = this.storageArrays[var2 >> 4];
			boolean var10 = false;
			if(var9 == null) {
				if(var4 == 0) {
					return false;
				}

				var9 = this.storageArrays[var2 >> 4] = new ExtendedBlockStorage(var2 >> 4 << 4);
				var10 = var2 >= var7;
			}

			var9.setExtBlockID(var1, var2 & 15, var3, var4);
			int var11 = this.xPosition * 16 + var1;
			int var12 = this.zPosition * 16 + var3;
			if(var8 != 0) {
				if(!this.worldObj.isRemote) {
					Block.blocksList[var8].onBlockRemoval(this.worldObj, var11, var2, var12);
				} else if(Block.blocksList[var8] instanceof BlockContainer && var8 != var4) {
					this.worldObj.removeBlockTileEntity(var11, var2, var12);
				}
			}

			if(var9.getExtBlockID(var1, var2 & 15, var3) != var4) {
				return false;
			} else {
				var9.setExtBlockMetadata(var1, var2 & 15, var3, var5);
				if(var10) {
					this.generateSkylightMap();
				} else {
					if(Block.lightOpacity[var4 & 4095] > 0) {
						if(var2 >= var7) {
							this.relightBlock(var1, var2 + 1, var3);
						}
					} else if(var2 == var7 - 1) {
						this.relightBlock(var1, var2, var3);
					}

					this.propagateSkylightOcclusion(var1, var3);
				}

				TileEntity var13;
				if(var4 != 0) {
					if(!this.worldObj.isRemote) {
						Block.blocksList[var4].onBlockAdded(this.worldObj, var11, var2, var12);
					}

					if(Block.blocksList[var4] instanceof BlockContainer) {
						var13 = this.getChunkBlockTileEntity(var1, var2, var3);
						if(var13 == null) {
							var13 = ((BlockContainer)Block.blocksList[var4]).getBlockEntity();
							this.worldObj.setBlockTileEntity(var11, var2, var12, var13);
						}

						if(var13 != null) {
							var13.updateContainingBlockInfo();
						}
					}
				} else if(var8 > 0 && Block.blocksList[var8] instanceof BlockContainer) {
					var13 = this.getChunkBlockTileEntity(var1, var2, var3);
					if(var13 != null) {
						var13.updateContainingBlockInfo();
					}
				}

				this.isModified = true;
				return true;
			}
		}
	}

	public boolean setBlockMetadata(int var1, int var2, int var3, int var4) {
		ExtendedBlockStorage var5 = this.storageArrays[var2 >> 4];
		if(var5 == null) {
			return false;
		} else {
			int var6 = var5.getExtBlockMetadata(var1, var2 & 15, var3);
			if(var6 == var4) {
				return false;
			} else {
				this.isModified = true;
				var5.setExtBlockMetadata(var1, var2 & 15, var3, var4);
				int var7 = var5.getExtBlockID(var1, var2 & 15, var3);
				if(var7 > 0 && Block.blocksList[var7] instanceof BlockContainer) {
					TileEntity var8 = this.getChunkBlockTileEntity(var1, var2, var3);
					if(var8 != null) {
						var8.updateContainingBlockInfo();
						var8.blockMetadata = var4;
					}
				}

				return true;
			}
		}
	}

	public int getSavedLightValue(EnumSkyBlock var1, int var2, int var3, int var4) {
		ExtendedBlockStorage var5 = this.storageArrays[var3 >> 4];
		return var5 == null ? var1.defaultLightValue : (var1 == EnumSkyBlock.Sky ? var5.getExtSkylightValue(var2, var3 & 15, var4) : (var1 == EnumSkyBlock.Block ? var5.getExtBlocklightValue(var2, var3 & 15, var4) : var1.defaultLightValue));
	}

	public void setLightValue(EnumSkyBlock var1, int var2, int var3, int var4, int var5) {
		ExtendedBlockStorage var6 = this.storageArrays[var3 >> 4];
		if(var6 == null) {
			var6 = this.storageArrays[var3 >> 4] = new ExtendedBlockStorage(var3 >> 4 << 4);
			this.generateSkylightMap();
		}

		this.isModified = true;
		if(var1 == EnumSkyBlock.Sky) {
			if(!this.worldObj.worldProvider.hasNoSky) {
				var6.setExtSkylightValue(var2, var3 & 15, var4, var5);
			}
		} else {
			if(var1 != EnumSkyBlock.Block) {
				return;
			}

			var6.setExtBlocklightValue(var2, var3 & 15, var4, var5);
		}

	}

	public int getBlockLightValue(int var1, int var2, int var3, int var4) {
		ExtendedBlockStorage var5 = this.storageArrays[var2 >> 4];
		if(var5 == null) {
			return !this.worldObj.worldProvider.hasNoSky && var4 < EnumSkyBlock.Sky.defaultLightValue ? EnumSkyBlock.Sky.defaultLightValue - var4 : 0;
		} else {
			int var6 = this.worldObj.worldProvider.hasNoSky ? 0 : var5.getExtSkylightValue(var1, var2 & 15, var3);
			if(var6 > 0) {
				isLit = true;
			}

			var6 -= var4;
			int var7 = var5.getExtBlocklightValue(var1, var2 & 15, var3);
			if(var7 > var6) {
				var6 = var7;
			}

			return var6;
		}
	}

	public void addEntity(Entity var1) {
		this.hasEntities = true;
		int var2 = MathHelper.floor_double(var1.posX / 16.0D);
		int var3 = MathHelper.floor_double(var1.posZ / 16.0D);
		if(var2 != this.xPosition || var3 != this.zPosition) {
			System.out.println("Wrong location! " + var1);
			Thread.dumpStack();
		}

		int var4 = MathHelper.floor_double(var1.posY / 16.0D);
		if(var4 < 0) {
			var4 = 0;
		}

		if(var4 >= this.entityLists.length) {
			var4 = this.entityLists.length - 1;
		}

		var1.addedToChunk = true;
		var1.chunkCoordX = this.xPosition;
		var1.chunkCoordY = var4;
		var1.chunkCoordZ = this.zPosition;
		this.entityLists[var4].add(var1);
	}

	public void removeEntity(Entity var1) {
		this.removeEntityAtIndex(var1, var1.chunkCoordY);
	}

	public void removeEntityAtIndex(Entity var1, int var2) {
		if(var2 < 0) {
			var2 = 0;
		}

		if(var2 >= this.entityLists.length) {
			var2 = this.entityLists.length - 1;
		}

		this.entityLists[var2].remove(var1);
	}

	public boolean canBlockSeeTheSky(int var1, int var2, int var3) {
		return var2 >= this.heightMap[var3 << 4 | var1];
	}

	public TileEntity getChunkBlockTileEntity(int var1, int var2, int var3) {
		ChunkPosition var4 = new ChunkPosition(var1, var2, var3);
		TileEntity var5 = (TileEntity)this.chunkTileEntityMap.get(var4);
		if(var5 == null) {
			int var6 = this.getBlockID(var1, var2, var3);
			if(var6 <= 0 || !Block.blocksList[var6].hasTileEntity()) {
				return null;
			}

			if(var5 == null) {
				var5 = ((BlockContainer)Block.blocksList[var6]).getBlockEntity();
				this.worldObj.setBlockTileEntity(this.xPosition * 16 + var1, var2, this.zPosition * 16 + var3, var5);
			}

			var5 = (TileEntity)this.chunkTileEntityMap.get(var4);
		}

		if(var5 != null && var5.isInvalid()) {
			this.chunkTileEntityMap.remove(var4);
			return null;
		} else {
			return var5;
		}
	}

	public void addTileEntity(TileEntity var1) {
		int var2 = var1.xCoord - this.xPosition * 16;
		int var3 = var1.yCoord;
		int var4 = var1.zCoord - this.zPosition * 16;
		this.setChunkBlockTileEntity(var2, var3, var4, var1);
		if(this.isChunkLoaded) {
			this.worldObj.loadedTileEntityList.add(var1);
		}

	}

	public void setChunkBlockTileEntity(int var1, int var2, int var3, TileEntity var4) {
		ChunkPosition var5 = new ChunkPosition(var1, var2, var3);
		var4.worldObj = this.worldObj;
		var4.xCoord = this.xPosition * 16 + var1;
		var4.yCoord = var2;
		var4.zCoord = this.zPosition * 16 + var3;
		if(this.getBlockID(var1, var2, var3) != 0 && Block.blocksList[this.getBlockID(var1, var2, var3)] instanceof BlockContainer) {
			var4.validate();
			this.chunkTileEntityMap.put(var5, var4);
		}
	}

	public void removeChunkBlockTileEntity(int var1, int var2, int var3) {
		ChunkPosition var4 = new ChunkPosition(var1, var2, var3);
		if(this.isChunkLoaded) {
			TileEntity var5 = (TileEntity)this.chunkTileEntityMap.remove(var4);
			if(var5 != null) {
				var5.invalidate();
			}
		}

	}

	public void onChunkLoad() {
		this.isChunkLoaded = true;
		this.worldObj.addTileEntity(this.chunkTileEntityMap.values());

		for(int var1 = 0; var1 < this.entityLists.length; ++var1) {
			this.worldObj.addLoadedEntities(this.entityLists[var1]);
		}

	}

	public void onChunkUnload() {
		this.isChunkLoaded = false;
		Iterator var1 = this.chunkTileEntityMap.values().iterator();

		while(var1.hasNext()) {
			TileEntity var2 = (TileEntity)var1.next();
			this.worldObj.markTileEntityForDespawn(var2);
		}

		for(int var3 = 0; var3 < this.entityLists.length; ++var3) {
			this.worldObj.unloadEntities(this.entityLists[var3]);
		}

	}

	public void setChunkModified() {
		this.isModified = true;
	}

	public void getEntitiesWithinAABBForEntity(Entity var1, AxisAlignedBB var2, List var3) {
		int var4 = MathHelper.floor_double((var2.minY - 2.0D) / 16.0D);
		int var5 = MathHelper.floor_double((var2.maxY + 2.0D) / 16.0D);
		if(var4 < 0) {
			var4 = 0;
		}

		if(var5 >= this.entityLists.length) {
			var5 = this.entityLists.length - 1;
		}

		for(int var6 = var4; var6 <= var5; ++var6) {
			List var7 = this.entityLists[var6];

			for(int var8 = 0; var8 < var7.size(); ++var8) {
				Entity var9 = (Entity)var7.get(var8);
				if(var9 != var1 && var9.boundingBox.intersectsWith(var2)) {
					var3.add(var9);
					Entity[] var10 = var9.getParts();
					if(var10 != null) {
						for(int var11 = 0; var11 < var10.length; ++var11) {
							var9 = var10[var11];
							if(var9 != var1 && var9.boundingBox.intersectsWith(var2)) {
								var3.add(var9);
							}
						}
					}
				}
			}
		}

	}

	public void getEntitiesOfTypeWithinAAAB(Class var1, AxisAlignedBB var2, List var3) {
		int var4 = MathHelper.floor_double((var2.minY - 2.0D) / 16.0D);
		int var5 = MathHelper.floor_double((var2.maxY + 2.0D) / 16.0D);
		if(var4 < 0) {
			var4 = 0;
		} else if(var4 >= this.entityLists.length) {
			var4 = this.entityLists.length - 1;
		}

		if(var5 >= this.entityLists.length) {
			var5 = this.entityLists.length - 1;
		} else if(var5 < 0) {
			var5 = 0;
		}

		for(int var6 = var4; var6 <= var5; ++var6) {
			List var7 = this.entityLists[var6];

			for(int var8 = 0; var8 < var7.size(); ++var8) {
				Entity var9 = (Entity)var7.get(var8);
				if(var1.isAssignableFrom(var9.getClass()) && var9.boundingBox.intersectsWith(var2)) {
					var3.add(var9);
				}
			}
		}

	}

	public boolean needsSaving(boolean var1) {
		if(var1) {
			if(this.hasEntities && this.worldObj.getWorldTime() != this.lastSaveTime) {
				return true;
			}
		} else if(this.hasEntities && this.worldObj.getWorldTime() >= this.lastSaveTime + 600L) {
			return true;
		}

		return this.isModified;
	}

	public Random getRandomWithSeed(long var1) {
		return new Random(this.worldObj.getSeed() + (long)(this.xPosition * this.xPosition * 4987142) + (long)(this.xPosition * 5947611) + (long)(this.zPosition * this.zPosition) * 4392871L + (long)(this.zPosition * 389711) ^ var1);
	}

	public boolean isEmpty() {
		return false;
	}

	public void removeUnknownBlocks() {
		ExtendedBlockStorage[] var1 = this.storageArrays;
		int var2 = var1.length;

		for(int var3 = 0; var3 < var2; ++var3) {
			ExtendedBlockStorage var4 = var1[var3];
			if(var4 != null) {
				var4.func_48711_e();
			}
		}

	}

	public void populateChunk(IChunkProvider var1, IChunkProvider var2, int var3, int var4) {
		if(!this.isTerrainPopulated && var1.chunkExists(var3 + 1, var4 + 1) && var1.chunkExists(var3, var4 + 1) && var1.chunkExists(var3 + 1, var4)) {
			var1.populate(var2, var3, var4);
		}

		if(var1.chunkExists(var3 - 1, var4) && !var1.provideChunk(var3 - 1, var4).isTerrainPopulated && var1.chunkExists(var3 - 1, var4 + 1) && var1.chunkExists(var3, var4 + 1) && var1.chunkExists(var3 - 1, var4 + 1)) {
			var1.populate(var2, var3 - 1, var4);
		}

		if(var1.chunkExists(var3, var4 - 1) && !var1.provideChunk(var3, var4 - 1).isTerrainPopulated && var1.chunkExists(var3 + 1, var4 - 1) && var1.chunkExists(var3 + 1, var4 - 1) && var1.chunkExists(var3 + 1, var4)) {
			var1.populate(var2, var3, var4 - 1);
		}

		if(var1.chunkExists(var3 - 1, var4 - 1) && !var1.provideChunk(var3 - 1, var4 - 1).isTerrainPopulated && var1.chunkExists(var3, var4 - 1) && var1.chunkExists(var3 - 1, var4)) {
			var1.populate(var2, var3 - 1, var4 - 1);
		}

	}

	public int getPrecipitationHeight(int var1, int var2) {
		int var3 = var1 | var2 << 4;
		int var4 = this.precipitationHeightMap[var3];
		if(var4 == -999) {
			int var5 = this.getTopFilledSegment() + 15;
			var4 = -1;

			while(true) {
				while(var5 > 0 && var4 == -1) {
					int var6 = this.getBlockID(var1, var5, var2);
					Material var7 = var6 == 0 ? Material.air : Block.blocksList[var6].blockMaterial;
					if(!var7.blocksMovement() && !var7.isLiquid()) {
						--var5;
					} else {
						var4 = var5 + 1;
					}
				}

				this.precipitationHeightMap[var3] = var4;
				break;
			}
		}

		return var4;
	}

	public void updateSkylight() {
		if(this.isGapLightingUpdated && !this.worldObj.worldProvider.hasNoSky) {
			this.updateSkylight_do();
		}

	}

	public ChunkCoordIntPair getChunkCoordIntPair() {
		return new ChunkCoordIntPair(this.xPosition, this.zPosition);
	}

	public boolean getAreLevelsEmpty(int var1, int var2) {
		if(var1 < 0) {
			var1 = 0;
		}

		if(var2 >= 256) {
			var2 = 255;
		}

		for(int var3 = var1; var3 <= var2; var3 += 16) {
			ExtendedBlockStorage var4 = this.storageArrays[var3 >> 4];
			if(var4 != null && !var4.getIsEmpty()) {
				return false;
			}
		}

		return true;
	}

	public void setStorageArrays(ExtendedBlockStorage[] var1) {
		this.storageArrays = var1;
	}

	public void func_48494_a(byte[] var1, int var2, int var3, boolean var4) {
		int var5 = 0;

		int var6;
		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if((var2 & 1 << var6) != 0) {
				if(this.storageArrays[var6] == null) {
					this.storageArrays[var6] = new ExtendedBlockStorage(var6 << 4);
				}

				byte[] var7 = this.storageArrays[var6].func_48692_g();
				System.arraycopy(var1, var5, var7, 0, var7.length);
				var5 += var7.length;
			} else if(var4 && this.storageArrays[var6] != null) {
				this.storageArrays[var6] = null;
			}
		}

		NibbleArray var8;
		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if((var2 & 1 << var6) != 0 && this.storageArrays[var6] != null) {
				var8 = this.storageArrays[var6].func_48697_j();
				System.arraycopy(var1, var5, var8.data, 0, var8.data.length);
				var5 += var8.data.length;
			}
		}

		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if((var2 & 1 << var6) != 0 && this.storageArrays[var6] != null) {
				var8 = this.storageArrays[var6].getBlocklightArray();
				System.arraycopy(var1, var5, var8.data, 0, var8.data.length);
				var5 += var8.data.length;
			}
		}

		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if((var2 & 1 << var6) != 0 && this.storageArrays[var6] != null) {
				var8 = this.storageArrays[var6].getSkylightArray();
				System.arraycopy(var1, var5, var8.data, 0, var8.data.length);
				var5 += var8.data.length;
			}
		}

		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if((var3 & 1 << var6) != 0) {
				if(this.storageArrays[var6] == null) {
					var5 += 2048;
				} else {
					var8 = this.storageArrays[var6].getBlockMSBArray();
					if(var8 == null) {
						var8 = this.storageArrays[var6].createBlockMSBArray();
					}

					System.arraycopy(var1, var5, var8.data, 0, var8.data.length);
					var5 += var8.data.length;
				}
			} else if(var4 && this.storageArrays[var6] != null && this.storageArrays[var6].getBlockMSBArray() != null) {
				this.storageArrays[var6].func_48715_h();
			}
		}

		if(var4) {
			System.arraycopy(var1, var5, this.blockBiomeArray, 0, this.blockBiomeArray.length);
			int var10000 = var5 + this.blockBiomeArray.length;
		}

		for(var6 = 0; var6 < this.storageArrays.length; ++var6) {
			if(this.storageArrays[var6] != null && (var2 & 1 << var6) != 0) {
				this.storageArrays[var6].func_48708_d();
			}
		}

		this.generateHeightMap();
		Iterator var10 = this.chunkTileEntityMap.values().iterator();

		while(var10.hasNext()) {
			TileEntity var9 = (TileEntity)var10.next();
			var9.updateContainingBlockInfo();
		}

	}

	public BiomeGenBase func_48490_a(int var1, int var2, WorldChunkManager var3) {
		int var4 = this.blockBiomeArray[var2 << 4 | var1] & 255;
		if(var4 == 255) {
			BiomeGenBase var5 = var3.getBiomeGenAt((this.xPosition << 4) + var1, (this.zPosition << 4) + var2);
			var4 = var5.biomeID;
			this.blockBiomeArray[var2 << 4 | var1] = (byte)(var4 & 255);
		}

		return BiomeGenBase.biomeList[var4] == null ? BiomeGenBase.plains : BiomeGenBase.biomeList[var4];
	}

	public byte[] getBiomeArray() {
		return this.blockBiomeArray;
	}

	public void setBiomeArray(byte[] var1) {
		this.blockBiomeArray = var1;
	}

	public void resetRelightChecks() {
		this.queuedLightChecks = 0;
	}

	public void enqueueRelightChecks() {
		for(int var1 = 0; var1 < 8; ++var1) {
			if(this.queuedLightChecks >= 4096) {
				return;
			}

			int var2 = this.queuedLightChecks % 16;
			int var3 = this.queuedLightChecks / 16 % 16;
			int var4 = this.queuedLightChecks / 256;
			++this.queuedLightChecks;
			int var5 = (this.xPosition << 4) + var3;
			int var6 = (this.zPosition << 4) + var4;

			for(int var7 = 0; var7 < 16; ++var7) {
				int var8 = (var2 << 4) + var7;
				if(this.storageArrays[var2] == null && (var7 == 0 || var7 == 15 || var3 == 0 || var3 == 15 || var4 == 0 || var4 == 15) || this.storageArrays[var2] != null && this.storageArrays[var2].getExtBlockID(var3, var7, var4) == 0) {
					if(Block.lightValue[this.worldObj.getBlockId(var5, var8 - 1, var6)] > 0) {
						this.worldObj.updateAllLightTypes(var5, var8 - 1, var6);
					}

					if(Block.lightValue[this.worldObj.getBlockId(var5, var8 + 1, var6)] > 0) {
						this.worldObj.updateAllLightTypes(var5, var8 + 1, var6);
					}

					if(Block.lightValue[this.worldObj.getBlockId(var5 - 1, var8, var6)] > 0) {
						this.worldObj.updateAllLightTypes(var5 - 1, var8, var6);
					}

					if(Block.lightValue[this.worldObj.getBlockId(var5 + 1, var8, var6)] > 0) {
						this.worldObj.updateAllLightTypes(var5 + 1, var8, var6);
					}

					if(Block.lightValue[this.worldObj.getBlockId(var5, var8, var6 - 1)] > 0) {
						this.worldObj.updateAllLightTypes(var5, var8, var6 - 1);
					}

					if(Block.lightValue[this.worldObj.getBlockId(var5, var8, var6 + 1)] > 0) {
						this.worldObj.updateAllLightTypes(var5, var8, var6 + 1);
					}

					this.worldObj.updateAllLightTypes(var5, var8, var6);
				}
			}
		}

	}
}
