package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentStrongholdChestCorridor extends ComponentStronghold {
	private static final StructurePieceTreasure[] chestLoot = new StructurePieceTreasure[]{new StructurePieceTreasure(Item.enderPearl.shiftedIndex, 0, 1, 1, 10), new StructurePieceTreasure(Item.diamond.shiftedIndex, 0, 1, 3, 3), new StructurePieceTreasure(Item.ingotIron.shiftedIndex, 0, 1, 5, 10), new StructurePieceTreasure(Item.ingotGold.shiftedIndex, 0, 1, 3, 5), new StructurePieceTreasure(Item.redstone.shiftedIndex, 0, 4, 9, 5), new StructurePieceTreasure(Item.bread.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.appleRed.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.pickaxeSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.swordSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.plateSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.helmetSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.legsSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.bootsSteel.shiftedIndex, 0, 1, 1, 5), new StructurePieceTreasure(Item.appleGold.shiftedIndex, 0, 1, 1, 1)};
	private final EnumDoor doorType;
	private boolean hasMadeChest;

	public ComponentStrongholdChestCorridor(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.doorType = this.getRandomDoor(var2);
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		this.getNextComponentNormal((ComponentStrongholdStairs2)var1, var2, var3, 1, 1);
	}

	public static ComponentStrongholdChestCorridor findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, -1, 0, 5, 5, 7, var5);
		return canStrongholdGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentStrongholdChestCorridor(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			this.fillWithRandomizedBlocks(var1, var3, 0, 0, 0, 4, 4, 6, true, var2, StructureStrongholdPieces.getStrongholdStones());
			this.placeDoor(var1, var2, var3, this.doorType, 1, 1, 0);
			this.placeDoor(var1, var2, var3, EnumDoor.OPENING, 1, 1, 6);
			this.fillWithBlocks(var1, var3, 3, 1, 2, 3, 1, 4, Block.stoneBrick.blockID, Block.stoneBrick.blockID, false);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 5, 3, 1, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 5, 3, 1, 5, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 5, 3, 2, 2, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 5, 3, 2, 4, var3);

			int var4;
			for(var4 = 2; var4 <= 4; ++var4) {
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 5, 2, 1, var4, var3);
			}

			if(!this.hasMadeChest) {
				var4 = this.getYWithOffset(2);
				int var5 = this.getXWithOffset(3, 3);
				int var6 = this.getZWithOffset(3, 3);
				if(var3.isVecInside(var5, var4, var6)) {
					this.hasMadeChest = true;
					this.createTreasureChestAtCurrentPosition(var1, var3, var2, 3, 2, 3, chestLoot, 2 + var2.nextInt(2));
				}
			}

			return true;
		}
	}
}
