package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

class StructureNetherBridgeStart extends StructureStart {
	public StructureNetherBridgeStart(World var1, Random var2, int var3, int var4) {
		ComponentNetherBridgeStartPiece var5 = new ComponentNetherBridgeStartPiece(var2, (var3 << 4) + 2, (var4 << 4) + 2);
		this.components.add(var5);
		var5.buildComponent(var5, this.components, var2);
		ArrayList var6 = var5.field_40034_d;

		while(!var6.isEmpty()) {
			int var7 = var2.nextInt(var6.size());
			StructureComponent var8 = (StructureComponent)var6.remove(var7);
			var8.buildComponent(var5, this.components, var2);
		}

		this.updateBoundingBox();
		this.setRandomHeight(var1, var2, 48, 70);
	}
}
