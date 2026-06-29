package net.minecraft.src;

import java.io.File;
import java.util.List;

public class AnvilSaveHandler extends SaveHandler {
	public AnvilSaveHandler(File var1, String var2, boolean var3) {
		super(var1, var2, var3);
	}

	public IChunkLoader getChunkLoader(WorldProvider var1) {
		File var2 = this.getSaveDirectory();
		File var3;
		if(var1 instanceof WorldProviderHell) {
			var3 = new File(var2, "DIM-1");
			var3.mkdirs();
			return new AnvilChunkLoader(var3);
		} else if(var1 instanceof WorldProviderEnd) {
			var3 = new File(var2, "DIM1");
			var3.mkdirs();
			return new AnvilChunkLoader(var3);
		} else {
			return new AnvilChunkLoader(var2);
		}
	}

	public void saveWorldInfoAndPlayer(WorldInfo var1, List var2) {
		var1.setSaveVersion(19133);
		super.saveWorldInfoAndPlayer(var1, var2);
	}
}
