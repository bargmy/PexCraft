package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentVillageWell extends ComponentVillage {
	private final boolean field_35104_a = true;
	private int averageGroundLevel = -1;

	public ComponentVillageWell(int var1, Random var2, int var3, int var4) {
		super(var1);
		this.coordBaseMode = var2.nextInt(4);
		switch(this.coordBaseMode) {
		case 0:
		case 2:
			this.boundingBox = new StructureBoundingBox(var3, 64, var4, var3 + 6 - 1, 78, var4 + 6 - 1);
			break;
		default:
			this.boundingBox = new StructureBoundingBox(var3, 64, var4, var3 + 6 - 1, 78, var4 + 6 - 1);
		}

	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.maxY - 4, this.boundingBox.minZ + 1, 1, this.getComponentType());
		StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.maxY - 4, this.boundingBox.minZ + 1, 3, this.getComponentType());
		StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.maxY - 4, this.boundingBox.minZ - 1, 2, this.getComponentType());
		StructureVillagePieces.getNextStructureComponentVillagePath((ComponentVillageStartPiece)var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.maxY - 4, this.boundingBox.maxZ + 1, 0, this.getComponentType());
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.averageGroundLevel < 0) {
			this.averageGroundLevel = this.getAverageGroundLevel(var1, var3);
			if(this.averageGroundLevel < 0) {
				return true;
			}

			this.boundingBox.offset(0, this.averageGroundLevel - this.boundingBox.maxY + 3, 0);
		}

		if(this.field_35104_a) {
		}

		this.fillWithBlocks(var1, var3, 1, 0, 1, 4, 12, 4, Block.cobblestone.blockID, Block.waterMoving.blockID, false);
		this.placeBlockAtCurrentPosition(var1, 0, 0, 2, 12, 2, var3);
		this.placeBlockAtCurrentPosition(var1, 0, 0, 3, 12, 2, var3);
		this.placeBlockAtCurrentPosition(var1, 0, 0, 2, 12, 3, var3);
		this.placeBlockAtCurrentPosition(var1, 0, 0, 3, 12, 3, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 13, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 14, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 4, 13, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 4, 14, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 13, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 14, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 4, 13, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 4, 14, 4, var3);
		this.fillWithBlocks(var1, var3, 1, 15, 1, 4, 15, 4, Block.cobblestone.blockID, Block.cobblestone.blockID, false);

		for(int var4 = 0; var4 <= 5; ++var4) {
			for(int var5 = 0; var5 <= 5; ++var5) {
				if(var5 == 0 || var5 == 5 || var4 == 0 || var4 == 5) {
					this.placeBlockAtCurrentPosition(var1, Block.gravel.blockID, 0, var5, 11, var4, var3);
					this.clearCurrentPositionBlocksUpwards(var1, var5, 12, var4, var3);
				}
			}
		}

		return true;
	}
}
