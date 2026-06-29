package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentVillageChurch extends ComponentVillage {
	private int averageGroundLevel = -1;

	public ComponentVillageChurch(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
	}

	public static ComponentVillageChurch findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, 0, 0, 0, 5, 12, 9, var5);
		return canVillageGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentVillageChurch(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.averageGroundLevel < 0) {
			this.averageGroundLevel = this.getAverageGroundLevel(var1, var3);
			if(this.averageGroundLevel < 0) {
				return true;
			}

			this.boundingBox.offset(0, this.averageGroundLevel - this.boundingBox.maxY + 12 - 1, 0);
		}

		this.fillWithBlocks(var1, var3, 1, 1, 1, 3, 3, 7, 0, 0, false);
		this.fillWithBlocks(var1, var3, 1, 5, 1, 3, 9, 3, 0, 0, false);
		this.fillWithBlocks(var1, var3, 1, 0, 0, 3, 0, 8, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 1, 0, 3, 10, 0, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 1, 1, 0, 10, 3, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 4, 1, 1, 4, 10, 3, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 0, 4, 0, 4, 7, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 4, 0, 4, 4, 4, 7, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 1, 8, 3, 4, 8, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 5, 4, 3, 10, 4, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 5, 5, 3, 5, 7, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 9, 0, 4, 9, 4, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 4, 0, 4, 4, 4, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 0, 11, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 4, 11, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 2, 11, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 2, 11, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 1, 1, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 1, 1, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 2, 1, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 3, 1, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 3, 1, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 3), 1, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 3), 2, 1, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 3), 3, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 1), 1, 2, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 0), 3, 2, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 2, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 3, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 2, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 3, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 6, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 7, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 6, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 7, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 6, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 7, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 6, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 7, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 3, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 3, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 3, 8, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 2, 4, 7, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 1, 4, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 3, 4, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 2, 4, 5, var3);
		int var4 = this.getMetadataWithOffset(Block.ladder.blockID, 4);

		int var5;
		for(var5 = 1; var5 <= 9; ++var5) {
			this.placeBlockAtCurrentPosition(var1, Block.ladder.blockID, var4, 3, var5, 3, var3);
		}

		this.placeBlockAtCurrentPosition(var1, 0, 0, 2, 1, 0, var3);
		this.placeBlockAtCurrentPosition(var1, 0, 0, 2, 2, 0, var3);
		this.placeDoorAtCurrentPosition(var1, var3, var2, 2, 1, 0, this.getMetadataWithOffset(Block.doorWood.blockID, 1));
		if(this.getBlockIdAtCurrentPosition(var1, 2, 0, -1, var3) == 0 && this.getBlockIdAtCurrentPosition(var1, 2, -1, -1, var3) != 0) {
			this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 3), 2, 0, -1, var3);
		}

		for(var5 = 0; var5 < 9; ++var5) {
			for(int var6 = 0; var6 < 5; ++var6) {
				this.clearCurrentPositionBlocksUpwards(var1, var6, 12, var5, var3);
				this.fillCurrentPositionBlocksDownwards(var1, Block.cobblestone.blockID, 0, var6, -1, var5, var3);
			}
		}

		this.spawnVillagers(var1, var3, 2, 1, 2, 1);
		return true;
	}

	protected int getVillagerType(int var1) {
		return 2;
	}
}
