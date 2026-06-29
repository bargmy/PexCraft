package net.minecraft.src;

import java.util.Random;

public abstract class WorldGenerator {
	private final boolean doBlockNotify;

	public WorldGenerator() {
		this.doBlockNotify = false;
	}

	public WorldGenerator(boolean var1) {
		this.doBlockNotify = var1;
	}

	public abstract boolean generate(World var1, Random var2, int var3, int var4, int var5);

	public void setScale(double var1, double var3, double var5) {
	}

	protected void func_50073_a(World var1, int var2, int var3, int var4, int var5) {
		this.setBlockAndMetadata(var1, var2, var3, var4, var5, 0);
	}

	protected void setBlockAndMetadata(World var1, int var2, int var3, int var4, int var5, int var6) {
		if(this.doBlockNotify) {
			var1.setBlockAndMetadataWithNotify(var2, var3, var4, var5, var6);
		} else if(var1.blockExists(var2, var3, var4) && var1.getChunkFromBlockCoords(var2, var4).field_50120_o) {
			if(var1.setBlockAndMetadata(var2, var3, var4, var5, var6)) {
				var1.markBlockNeedsUpdate(var2, var3, var4);
			}
		} else {
			var1.setBlockAndMetadata(var2, var3, var4, var5, var6);
		}

	}
}
