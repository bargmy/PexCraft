package net.minecraft.src;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

public class AnvilChunkLoader implements IThreadedFileIO, IChunkLoader {
	private List field_48451_a = new ArrayList();
	private Set field_48449_b = new HashSet();
	private Object field_48450_c = new Object();
	private final File chunkSaveLocation;

	public AnvilChunkLoader(File var1) {
		this.chunkSaveLocation = var1;
	}

	public Chunk loadChunk(World var1, int var2, int var3) throws IOException {
		NBTTagCompound var4 = null;
		ChunkCoordIntPair var5 = new ChunkCoordIntPair(var2, var3);
		Object var6 = this.field_48450_c;
		synchronized(var6) {
			if(this.field_48449_b.contains(var5)) {
				for(int var7 = 0; var7 < this.field_48451_a.size(); ++var7) {
					if(((AnvilChunkLoaderPending)this.field_48451_a.get(var7)).field_48427_a.equals(var5)) {
						var4 = ((AnvilChunkLoaderPending)this.field_48451_a.get(var7)).field_48426_b;
						break;
					}
				}
			}
		}

		if(var4 == null) {
			DataInputStream var10 = RegionFileCache.getChunkInputStream(this.chunkSaveLocation, var2, var3);
			if(var10 == null) {
				return null;
			}

			var4 = CompressedStreamTools.read((DataInput)var10);
		}

		return this.func_48443_a(var1, var2, var3, var4);
	}

	protected Chunk func_48443_a(World var1, int var2, int var3, NBTTagCompound var4) {
		if(!var4.hasKey("Level")) {
			System.out.println("Chunk file at " + var2 + "," + var3 + " is missing level data, skipping");
			return null;
		} else if(!var4.getCompoundTag("Level").hasKey("Sections")) {
			System.out.println("Chunk file at " + var2 + "," + var3 + " is missing block data, skipping");
			return null;
		} else {
			Chunk var5 = this.func_48444_a(var1, var4.getCompoundTag("Level"));
			if(!var5.isAtLocation(var2, var3)) {
				System.out.println("Chunk file at " + var2 + "," + var3 + " is in the wrong location; relocating. (Expected " + var2 + ", " + var3 + ", got " + var5.xPosition + ", " + var5.zPosition + ")");
				var4.setInteger("xPos", var2);
				var4.setInteger("zPos", var3);
				var5 = this.func_48444_a(var1, var4.getCompoundTag("Level"));
			}

			var5.removeUnknownBlocks();
			return var5;
		}
	}

	public void saveChunk(World var1, Chunk var2) {
		var1.checkSessionLock();

		try {
			NBTTagCompound var3 = new NBTTagCompound();
			NBTTagCompound var4 = new NBTTagCompound();
			var3.setTag("Level", var4);
			this.func_48445_a(var2, var1, var4);
			this.func_48446_a(var2.getChunkCoordIntPair(), var3);
		} catch (Exception var5) {
			var5.printStackTrace();
		}

	}

	protected void func_48446_a(ChunkCoordIntPair var1, NBTTagCompound var2) {
		Object var3 = this.field_48450_c;
		synchronized(var3) {
			if(this.field_48449_b.contains(var1)) {
				for(int var4 = 0; var4 < this.field_48451_a.size(); ++var4) {
					if(((AnvilChunkLoaderPending)this.field_48451_a.get(var4)).field_48427_a.equals(var1)) {
						this.field_48451_a.set(var4, new AnvilChunkLoaderPending(var1, var2));
						return;
					}
				}
			}

			this.field_48451_a.add(new AnvilChunkLoaderPending(var1, var2));
			this.field_48449_b.add(var1);
			ThreadedFileIOBase.threadedIOInstance.queueIO(this);
		}
	}

	public boolean writeNextIO() {
		AnvilChunkLoaderPending var1 = null;
		Object var2 = this.field_48450_c;
		synchronized(var2) {
			if(this.field_48451_a.size() <= 0) {
				return false;
			}

			var1 = (AnvilChunkLoaderPending)this.field_48451_a.remove(0);
			this.field_48449_b.remove(var1.field_48427_a);
		}

		if(var1 != null) {
			try {
				this.func_48447_a(var1);
			} catch (Exception var4) {
				var4.printStackTrace();
			}
		}

		return true;
	}

	private void func_48447_a(AnvilChunkLoaderPending var1) throws IOException {
		DataOutputStream var2 = RegionFileCache.getChunkOutputStream(this.chunkSaveLocation, var1.field_48427_a.chunkXPos, var1.field_48427_a.chunkZPos);
		CompressedStreamTools.write(var1.field_48426_b, (DataOutput)var2);
		var2.close();
	}

	public void saveExtraChunkData(World var1, Chunk var2) {
	}

	public void chunkTick() {
	}

	public void saveExtraData() {
	}

	private void func_48445_a(Chunk var1, World var2, NBTTagCompound var3) {
		var2.checkSessionLock();
		var3.setInteger("xPos", var1.xPosition);
		var3.setInteger("zPos", var1.zPosition);
		var3.setLong("LastUpdate", var2.getWorldTime());
		var3.func_48183_a("HeightMap", var1.heightMap);
		var3.setBoolean("TerrainPopulated", var1.isTerrainPopulated);
		ExtendedBlockStorage[] var4 = var1.getBlockStorageArray();
		NBTTagList var5 = new NBTTagList("Sections");
		ExtendedBlockStorage[] var6 = var4;
		int var7 = var4.length;

		NBTTagCompound var10;
		for(int var8 = 0; var8 < var7; ++var8) {
			ExtendedBlockStorage var9 = var6[var8];
			if(var9 != null && var9.func_48700_f() != 0) {
				var10 = new NBTTagCompound();
				var10.setByte("Y", (byte)(var9.getYLocation() >> 4 & 255));
				var10.setByteArray("Blocks", var9.func_48692_g());
				if(var9.getBlockMSBArray() != null) {
					var10.setByteArray("Add", var9.getBlockMSBArray().data);
				}

				var10.setByteArray("Data", var9.func_48697_j().data);
				var10.setByteArray("SkyLight", var9.getSkylightArray().data);
				var10.setByteArray("BlockLight", var9.getBlocklightArray().data);
				var5.appendTag(var10);
			}
		}

		var3.setTag("Sections", var5);
		var3.setByteArray("Biomes", var1.getBiomeArray());
		var1.hasEntities = false;
		NBTTagList var15 = new NBTTagList();

		Iterator var17;
		for(var7 = 0; var7 < var1.entityLists.length; ++var7) {
			var17 = var1.entityLists[var7].iterator();

			while(var17.hasNext()) {
				Entity var19 = (Entity)var17.next();
				var1.hasEntities = true;
				var10 = new NBTTagCompound();
				if(var19.addEntityID(var10)) {
					var15.appendTag(var10);
				}
			}
		}

		var3.setTag("Entities", var15);
		NBTTagList var16 = new NBTTagList();
		var17 = var1.chunkTileEntityMap.values().iterator();

		while(var17.hasNext()) {
			TileEntity var20 = (TileEntity)var17.next();
			var10 = new NBTTagCompound();
			var20.writeToNBT(var10);
			var16.appendTag(var10);
		}

		var3.setTag("TileEntities", var16);
		List var18 = var2.getPendingBlockUpdates(var1, false);
		if(var18 != null) {
			long var21 = var2.getWorldTime();
			NBTTagList var11 = new NBTTagList();
			Iterator var12 = var18.iterator();

			while(var12.hasNext()) {
				NextTickListEntry var13 = (NextTickListEntry)var12.next();
				NBTTagCompound var14 = new NBTTagCompound();
				var14.setInteger("i", var13.blockID);
				var14.setInteger("x", var13.xCoord);
				var14.setInteger("y", var13.yCoord);
				var14.setInteger("z", var13.zCoord);
				var14.setInteger("t", (int)(var13.scheduledTime - var21));
				var11.appendTag(var14);
			}

			var3.setTag("TileTicks", var11);
		}

	}

	private Chunk func_48444_a(World var1, NBTTagCompound var2) {
		int var3 = var2.getInteger("xPos");
		int var4 = var2.getInteger("zPos");
		Chunk var5 = new Chunk(var1, var3, var4);
		var5.heightMap = var2.func_48182_l("HeightMap");
		var5.isTerrainPopulated = var2.getBoolean("TerrainPopulated");
		NBTTagList var6 = var2.getTagList("Sections");
		byte var7 = 16;
		ExtendedBlockStorage[] var8 = new ExtendedBlockStorage[var7];

		for(int var9 = 0; var9 < var6.tagCount(); ++var9) {
			NBTTagCompound var10 = (NBTTagCompound)var6.tagAt(var9);
			byte var11 = var10.getByte("Y");
			ExtendedBlockStorage var12 = new ExtendedBlockStorage(var11 << 4);
			var12.setBlockLSBArray(var10.getByteArray("Blocks"));
			if(var10.hasKey("Add")) {
				var12.setBlockMSBArray(new NibbleArray(var10.getByteArray("Add"), 4));
			}

			var12.setBlockMetadataArray(new NibbleArray(var10.getByteArray("Data"), 4));
			var12.setSkylightArray(new NibbleArray(var10.getByteArray("SkyLight"), 4));
			var12.setBlocklightArray(new NibbleArray(var10.getByteArray("BlockLight"), 4));
			var12.func_48708_d();
			var8[var11] = var12;
		}

		var5.setStorageArrays(var8);
		if(var2.hasKey("Biomes")) {
			var5.setBiomeArray(var2.getByteArray("Biomes"));
		}

		NBTTagList var14 = var2.getTagList("Entities");
		if(var14 != null) {
			for(int var15 = 0; var15 < var14.tagCount(); ++var15) {
				NBTTagCompound var17 = (NBTTagCompound)var14.tagAt(var15);
				Entity var19 = EntityList.createEntityFromNBT(var17, var1);
				var5.hasEntities = true;
				if(var19 != null) {
					var5.addEntity(var19);
				}
			}
		}

		NBTTagList var16 = var2.getTagList("TileEntities");
		if(var16 != null) {
			for(int var18 = 0; var18 < var16.tagCount(); ++var18) {
				NBTTagCompound var21 = (NBTTagCompound)var16.tagAt(var18);
				TileEntity var13 = TileEntity.createAndLoadEntity(var21);
				if(var13 != null) {
					var5.addTileEntity(var13);
				}
			}
		}

		if(var2.hasKey("TileTicks")) {
			NBTTagList var20 = var2.getTagList("TileTicks");
			if(var20 != null) {
				for(int var22 = 0; var22 < var20.tagCount(); ++var22) {
					NBTTagCompound var23 = (NBTTagCompound)var20.tagAt(var22);
					var1.scheduleBlockUpdateFromLoad(var23.getInteger("x"), var23.getInteger("y"), var23.getInteger("z"), var23.getInteger("i"), var23.getInteger("t"));
				}
			}
		}

		return var5;
	}
}
