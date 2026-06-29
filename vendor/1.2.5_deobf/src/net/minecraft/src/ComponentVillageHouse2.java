package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentVillageHouse2 extends ComponentVillage {
	private static final StructurePieceTreasure[] chestLoot = new StructurePieceTreasure[]{new StructurePieceTreasure(Item.diamond.shiftedIndex, 0, 1, 3, 3), new StructurePieceTreasure(Item.ingotIron.shiftedIndex, 0, 1, 5, 10), new StructurePieceTreasure(Item.ingotGold.shiftedIndex, 0, 1, 3, 5), new StructurePieceTreasure(Item.bread.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.appleRed.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.pickaxeSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.swordSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.plateSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.helmetSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.legsSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.bootsSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Block.obsidian.blockID, 0, 3, 7, 5), new StructurePieceTreasure(Block.sapling.blockID, 0, 3, 7, 5)};
	private int averageGroundLevel = -1;
	private boolean hasMadeChest;

	public ComponentVillageHouse2(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
	}

	public static ComponentVillageHouse2 findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, 0, 0, 0, 10, 6, 7, var5);
		return canVillageGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentVillageHouse2(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.averageGroundLevel < 0) {
			this.averageGroundLevel = this.getAverageGroundLevel(var1, var3);
			if(this.averageGroundLevel < 0) {
				return true;
			}

			this.boundingBox.offset(0, this.averageGroundLevel - this.boundingBox.maxY + 6 - 1, 0);
		}

		this.fillWithBlocks(var1, var3, 0, 1, 0, 9, 4, 6, 0, 0, false);
		this.fillWithBlocks(var1, var3, 0, 0, 0, 9, 0, 6, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 4, 0, 9, 4, 6, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 5, 0, 9, 5, 6, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 5, 1, 8, 5, 5, 0, 0, false);
		this.fillWithBlocks(var1, var3, 1, 1, 0, 2, 3, 0, Block.planks.blockID, Block.planks.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 1, 0, 0, 4, 0, Block.wood.blockID, Block.wood.blockID, false);
		this.fillWithBlocks(var1, var3, 3, 1, 0, 3, 4, 0, Block.wood.blockID, Block.wood.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 1, 6, 0, 4, 6, Block.wood.blockID, Block.wood.blockID, false);
		this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 3, 3, 1, var3);
		this.fillWithBlocks(var1, var3, 3, 1, 2, 3, 3, 2, Block.planks.blockID, Block.planks.blockID, false);
		this.fillWithBlocks(var1, var3, 4, 1, 3, 5, 3, 3, Block.planks.blockID, Block.planks.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 1, 1, 0, 3, 5, Block.planks.blockID, Block.planks.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 1, 6, 5, 3, 6, Block.planks.blockID, Block.planks.blockID, false);
		this.fillWithBlocks(var1, var3, 5, 1, 0, 5, 3, 0, Block.fence.blockID, Block.fence.blockID, false);
		this.fillWithBlocks(var1, var3, 9, 1, 0, 9, 3, 0, Block.fence.blockID, Block.fence.blockID, false);
		this.fillWithBlocks(var1, var3, 6, 1, 4, 9, 4, 6, Block.cobblestone.blockID, Block.cobblestone.blockID, false);
		this.placeBlockAtCurrentPosition(var1, Block.lavaMoving.blockID, 0, 7, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.lavaMoving.blockID, 0, 8, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, 9, 2, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, 9, 2, 4, var3);
		this.fillWithBlocks(var1, var3, 7, 2, 4, 8, 2, 5, 0, 0, false);
		this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 6, 1, 3, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stoneOvenIdle.blockID, 0, 6, 2, 3, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stoneOvenIdle.blockID, 0, 6, 3, 3, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairDouble.blockID, 0, 8, 1, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 2, 2, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 0, 2, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 2, 2, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.thinGlass.blockID, 0, 4, 2, 6, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 2, 1, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.pressurePlatePlanks.blockID, 0, 2, 2, 4, var3);
		this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 1, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactPlanks.blockID, this.getMetadataWithOffset(Block.stairCompactPlanks.blockID, 3), 2, 1, 5, var3);
		this.placeBlockAtCurrentPosition(var1, Block.stairCompactPlanks.blockID, this.getMetadataWithOffset(Block.stairCompactPlanks.blockID, 1), 1, 1, 4, var3);
		int var4;
		int var5;
		if(!this.hasMadeChest) {
			var4 = this.getYWithOffset(1);
			var5 = this.getXWithOffset(5, 5);
			int var6 = this.getZWithOffset(5, 5);
			if(var3.isVecInside(var5, var4, var6)) {
				this.hasMadeChest = true;
				this.createTreasureChestAtCurrentPosition(var1, var3, var2, 5, 1, 5, chestLoot, 3 + var2.nextInt(6));
			}
		}

		for(var4 = 6; var4 <= 8; ++var4) {
			if(this.getBlockIdAtCurrentPosition(var1, var4, 0, -1, var3) == 0 && this.getBlockIdAtCurrentPosition(var1, var4, -1, -1, var3) != 0) {
				this.placeBlockAtCurrentPosition(var1, Block.stairCompactCobblestone.blockID, this.getMetadataWithOffset(Block.stairCompactCobblestone.blockID, 3), var4, 0, -1, var3);
			}
		}

		for(var4 = 0; var4 < 7; ++var4) {
			for(var5 = 0; var5 < 10; ++var5) {
				this.clearCurrentPositionBlocksUpwards(var1, var5, 6, var4, var3);
				this.fillCurrentPositionBlocksDownwards(var1, Block.cobblestone.blockID, 0, var5, -1, var4, var3);
			}
		}

		this.spawnVillagers(var1, var3, 7, 1, 1, 1);
		return true;
	}

	protected int getVillagerType(int var1) {
		return 3;
	}
}
