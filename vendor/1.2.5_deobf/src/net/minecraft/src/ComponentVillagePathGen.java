package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentVillagePathGen extends ComponentVillageRoadPiece {
	private int averageGroundLevel;

	public ComponentVillagePathGen(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
		this.averageGroundLevel = Math.max(var3.getXSize(), var3.getZSize());
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		boolean var4 = false;

		int var5;
		StructureComponent var6;
		for(var5 = var3.nextInt(5); var5 < this.averageGroundLevel - 8; var5 += 2 + var3.nextInt(5)) {
			var6 = this.getNextComponentNN((ComponentVillageStartPiece)var1, var2, var3, 0, var5);
			if(var6 != null) {
				var5 += Math.max(var6.boundingBox.getXSize(), var6.boundingBox.getZSize());
				var4 = true;
			}
		}

		for(var5 = var3.nextInt(5); var5 < this.averageGroundLevel - 8; var5 += 2 + var3.nextInt(5)) {
			var6 = this.getNextComponentPP((ComponentVillageStartPiece)var1, var2, var3, 0, var5);
			if(var6 != null) {
				var5 += Math.max(var6.boundingBox.getXSize(), var6.boundingBox.getZSize());
				var4 = true;
			}
		}

		if(var4 && var3.nextInt(3) > 0) {
			switch(this.coordBaseMode) {
			case 0:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, this.boundingBox.maxZ - 2, 1, this.getComponentType());
				break;
			case 1:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, this.getComponentType());
				break;
			case 2:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, this.boundingBox.minZ, 1, this.getComponentType());
				break;
			case 3:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.maxX - 2, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, this.getComponentType());
			}
		}

		if(var4 && var3.nextInt(3) > 0) {
			switch(this.coordBaseMode) {
			case 0:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, this.boundingBox.maxZ - 2, 3, this.getComponentType());
				break;
			case 1:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, this.getComponentType());
				break;
			case 2:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, this.boundingBox.minZ, 3, this.getComponentType());
				break;
			case 3:
				StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.maxX - 2, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, this.getComponentType());
			}
		}

	}

	public static StructureBoundingBox func_35087_a(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6) {
		for(int var7 = 7 * MathHelper.getRandomIntegerInRange(var2, 3, 5); var7 >= 7; var7 -= 7) {
			StructureBoundingBox var8 = StructureBoundingBox.getComponentToAddBoundingBox(var3, var4, var5, 0, 0, 0, 3, 3, var7, var6);
			if(StructureComponent.findIntersecting(var1, var8) == null) {
				return var8;
			}
		}

		return null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		for(int var4 = this.boundingBox.minX; var4 <= this.boundingBox.maxX; ++var4) {
			for(int var5 = this.boundingBox.minZ; var5 <= this.boundingBox.maxZ; ++var5) {
				if(var3.isVecInside(var4, 64, var5)) {
					int var6 = var1.getTopSolidOrLiquidBlock(var4, var5) - 1;
					var1.setBlock(var4, var6, var5, Block.gravel.blockID);
				}
			}
		}

		return true;
	}
}
