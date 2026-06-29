package net.minecraft.src;

import java.util.Random;

public class StructureMineshaftStart extends StructureStart {
	public StructureMineshaftStart(World var1, Random var2, int var3, int var4) {
		ComponentMineshaftRoom var5 = new ComponentMineshaftRoom(0, var2, (var3 << 4) + 2, (var4 << 4) + 2);
		this.components.add(var5);
		var5.buildComponent(var5, this.components, var2);
		this.updateBoundingBox();
		this.markAvailableHeight(var1, var2, 10);
	}
}
