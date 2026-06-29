package net.minecraft.src;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Random;

class StructureVillageStart extends StructureStart {
	private boolean hasMoreThanTwoComponents = false;

	public StructureVillageStart(World var1, Random var2, int var3, int var4, int var5) {
		ArrayList var7 = StructureVillagePieces.getStructureVillageWeightedPieceList(var2, var5);
		ComponentVillageStartPiece var8 = new ComponentVillageStartPiece(var1.getWorldChunkManager(), 0, var2, (var3 << 4) + 2, (var4 << 4) + 2, var7, var5);
		this.components.add(var8);
		var8.buildComponent(var8, this.components, var2);
		ArrayList var9 = var8.field_35106_f;
		ArrayList var10 = var8.field_35108_e;

		int var11;
		while(!var9.isEmpty() || !var10.isEmpty()) {
			StructureComponent var12;
			if(!var9.isEmpty()) {
				var11 = var2.nextInt(var9.size());
				var12 = (StructureComponent)var9.remove(var11);
				var12.buildComponent(var8, this.components, var2);
			} else {
				var11 = var2.nextInt(var10.size());
				var12 = (StructureComponent)var10.remove(var11);
				var12.buildComponent(var8, this.components, var2);
			}
		}

		this.updateBoundingBox();
		var11 = 0;
		Iterator var14 = this.components.iterator();

		while(var14.hasNext()) {
			StructureComponent var13 = (StructureComponent)var14.next();
			if(!(var13 instanceof ComponentVillageRoadPiece)) {
				++var11;
			}
		}

		this.hasMoreThanTwoComponents = var11 > 2;
	}

	public boolean isSizeableStructure() {
		return this.hasMoreThanTwoComponents;
	}
}
