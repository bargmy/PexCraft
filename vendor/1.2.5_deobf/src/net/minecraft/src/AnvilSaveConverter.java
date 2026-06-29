package net.minecraft.src;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class AnvilSaveConverter extends SaveFormatOld {
	public AnvilSaveConverter(File var1) {
		super(var1);
	}

	public String getFormatName() {
		return "Anvil";
	}

	public List getSaveList() {
		ArrayList var1 = new ArrayList();
		File[] var2 = this.savesDirectory.listFiles();
		File[] var3 = var2;
		int var4 = var2.length;

		for(int var5 = 0; var5 < var4; ++var5) {
			File var6 = var3[var5];
			if(var6.isDirectory()) {
				String var7 = var6.getName();
				WorldInfo var8 = this.getWorldInfo(var7);
				if(var8 != null && (var8.getSaveVersion() == 19132 || var8.getSaveVersion() == 19133)) {
					boolean var9 = var8.getSaveVersion() != this.func_48431_c();
					String var10 = var8.getWorldName();
					if(var10 == null || MathHelper.stringNullOrLengthZero(var10)) {
						var10 = var7;
					}

					long var11 = 0L;
					var1.add(new SaveFormatComparator(var7, var10, var8.getLastTimePlayed(), var11, var8.getGameType(), var9, var8.isHardcoreModeEnabled()));
				}
			}
		}

		return var1;
	}

	protected int func_48431_c() {
		return 19133;
	}

	public void flushCache() {
		RegionFileCache.clearRegionFileReferences();
	}

	public ISaveHandler getSaveLoader(String var1, boolean var2) {
		return new AnvilSaveHandler(this.savesDirectory, var1, var2);
	}

	public boolean isOldMapFormat(String var1) {
		WorldInfo var2 = this.getWorldInfo(var1);
		return var2 != null && var2.getSaveVersion() != this.func_48431_c();
	}

	public boolean convertMapFormat(String var1, IProgressUpdate var2) {
		var2.setLoadingProgress(0);
		ArrayList var3 = new ArrayList();
		ArrayList var4 = new ArrayList();
		ArrayList var5 = new ArrayList();
		File var6 = new File(this.savesDirectory, var1);
		File var7 = new File(var6, "DIM-1");
		File var8 = new File(var6, "DIM1");
		System.out.println("Scanning folders...");
		this.func_48432_a(var6, var3);
		if(var7.exists()) {
			this.func_48432_a(var7, var4);
		}

		if(var8.exists()) {
			this.func_48432_a(var8, var5);
		}

		int var9 = var3.size() + var4.size() + var5.size();
		System.out.println("Total conversion count is " + var9);
		WorldInfo var10 = this.getWorldInfo(var1);
		Object var11 = null;
		if(var10.getTerrainType() == WorldType.FLAT) {
			var11 = new WorldChunkManagerHell(BiomeGenBase.plains, 0.5F, 0.5F);
		} else {
			var11 = new WorldChunkManager(var10.getSeed(), var10.getTerrainType());
		}

		this.func_48428_a(new File(var6, "region"), var3, (WorldChunkManager)var11, 0, var9, var2);
		this.func_48428_a(new File(var7, "region"), var4, new WorldChunkManagerHell(BiomeGenBase.hell, 1.0F, 0.0F), var3.size(), var9, var2);
		this.func_48428_a(new File(var8, "region"), var5, new WorldChunkManagerHell(BiomeGenBase.sky, 0.5F, 0.0F), var3.size() + var4.size(), var9, var2);
		var10.setSaveVersion(19133);
		if(var10.getTerrainType() == WorldType.DEFAULT_1_1) {
			var10.setTerrainType(WorldType.DEFAULT);
		}

		this.func_48429_d(var1);
		ISaveHandler var12 = this.getSaveLoader(var1, false);
		var12.saveWorldInfo(var10);
		return true;
	}

	private void func_48429_d(String var1) {
		File var2 = new File(this.savesDirectory, var1);
		if(!var2.exists()) {
			System.out.println("Warning: Unable to create level.dat_mcr backup");
		} else {
			File var3 = new File(var2, "level.dat");
			if(!var3.exists()) {
				System.out.println("Warning: Unable to create level.dat_mcr backup");
			} else {
				File var4 = new File(var2, "level.dat_mcr");
				if(!var3.renameTo(var4)) {
					System.out.println("Warning: Unable to create level.dat_mcr backup");
				}

			}
		}
	}

	private void func_48428_a(File var1, ArrayList var2, WorldChunkManager var3, int var4, int var5, IProgressUpdate var6) {
		Iterator var7 = var2.iterator();

		while(var7.hasNext()) {
			File var8 = (File)var7.next();
			this.func_48430_a(var1, var8, var3, var4, var5, var6);
			++var4;
			int var9 = (int)Math.round(100.0D * (double)var4 / (double)var5);
			var6.setLoadingProgress(var9);
		}

	}

	private void func_48430_a(File var1, File var2, WorldChunkManager var3, int var4, int var5, IProgressUpdate var6) {
		try {
			String var7 = var2.getName();
			RegionFile var8 = new RegionFile(var2);
			RegionFile var9 = new RegionFile(new File(var1, var7.substring(0, var7.length() - ".mcr".length()) + ".mca"));

			for(int var10 = 0; var10 < 32; ++var10) {
				int var11;
				for(var11 = 0; var11 < 32; ++var11) {
					if(var8.isChunkSaved(var10, var11) && !var9.isChunkSaved(var10, var11)) {
						DataInputStream var12 = var8.getChunkDataInputStream(var10, var11);
						if(var12 == null) {
							System.out.println("Failed to fetch input stream");
						} else {
							NBTTagCompound var13 = CompressedStreamTools.read((DataInput)var12);
							var12.close();
							NBTTagCompound var14 = var13.getCompoundTag("Level");
							AnvilConverterData var15 = ChunkLoader.load(var14);
							NBTTagCompound var16 = new NBTTagCompound();
							NBTTagCompound var17 = new NBTTagCompound();
							var16.setTag("Level", var17);
							ChunkLoader.convertToAnvilFormat(var15, var17, var3);
							DataOutputStream var18 = var9.getChunkDataOutputStream(var10, var11);
							CompressedStreamTools.write(var16, (DataOutput)var18);
							var18.close();
						}
					}
				}

				var11 = (int)Math.round(100.0D * (double)(var4 * 1024) / (double)(var5 * 1024));
				int var20 = (int)Math.round(100.0D * (double)((var10 + 1) * 32 + var4 * 1024) / (double)(var5 * 1024));
				if(var20 > var11) {
					var6.setLoadingProgress(var20);
				}
			}

			var8.close();
			var9.close();
		} catch (IOException var19) {
			var19.printStackTrace();
		}

	}

	private void func_48432_a(File var1, ArrayList var2) {
		File var3 = new File(var1, "region");
		File[] var4 = var3.listFiles(new AnvilSaveConverterFileFilter(this));
		if(var4 != null) {
			File[] var5 = var4;
			int var6 = var4.length;

			for(int var7 = 0; var7 < var6; ++var7) {
				File var8 = var5[var7];
				var2.add(var8);
			}
		}

	}
}
