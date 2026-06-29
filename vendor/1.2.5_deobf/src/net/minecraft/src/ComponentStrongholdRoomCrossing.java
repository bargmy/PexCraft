package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentStrongholdRoomCrossing extends ComponentStronghold {
	private static final StructurePieceTreasure[] chestLoot = new StructurePieceTreasure[]{new StructurePieceTreasure(Item.ingotIron.shiftedIndex, 0, 1, 5, 10), new StructurePieceTreasure(Item.ingotGold.shiftedIndex, 0, 1, 3, 5), new StructurePieceTreasure(Item.redstone.shiftedIndex, 0, 4, 9, 5), new StructurePieceTreasure(Item.coal.shiftedIndex, 0, 3, 8, 10), new StructurePieceTreasure(Item.bread.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.appleRed.shiftedIndex, 0, 1, 3, 15), new StructurePieceTreasure(Item.pickaxeSteel.shiftedIndex, 0, 1, 1, 1)};
	protected final EnumDoor doorType;
	protected final int roomType;

	public ComponentStrongholdRoomCrossing(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.doorType = this.getRandomDoor(var2);
		this.boundingBox = var3;
		this.roomType = var2.nextInt(5);
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		this.getNextComponentNormal((ComponentStrongholdStairs2)var1, var2, var3, 4, 1);
		this.getNextComponentX((ComponentStrongholdStairs2)var1, var2, var3, 1, 4);
		this.getNextComponentZ((ComponentStrongholdStairs2)var1, var2, var3, 1, 4);
	}

	public static ComponentStrongholdRoomCrossing findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -4, -1, 0, 11, 7, 11, var5);
		return canStrongholdGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentStrongholdRoomCrossing(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			this.fillWithRandomizedBlocks(var1, var3, 0, 0, 0, 10, 6, 10, true, var2, StructureStrongholdPieces.getStrongholdStones());
			this.placeDoor(var1, var2, var3, this.doorType, 4, 1, 0);
			this.fillWithBlocks(var1, var3, 4, 1, 10, 6, 3, 10, 0, 0, false);
			this.fillWithBlocks(var1, var3, 0, 1, 4, 0, 3, 6, 0, 0, false);
			this.fillWithBlocks(var1, var3, 10, 1, 4, 10, 3, 6, 0, 0, false);
			int var4;
			switch(this.roomType) {
			case 0:
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 2, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 3, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 4, 3, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 6, 3, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 5, 3, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 5, 3, 6, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 4, 1, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 4, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 4, 1, 6, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 6, 1, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 6, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 6, 1, 6, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 5, 1, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 5, 1, 6, var3);
				break;
			case 1:
				for(var4 = 0; var4 < 5; ++var4) {
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 1, 3 + var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 7, 1, 3 + var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3 + var4, 1, 3, var3);
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3 + var4, 1, 7, var3);
				}

				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 2, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 5, 3, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.waterMoving.blockID, 0, 5, 4, 5, var3);
				break;
			case 2:
				for(var4 = 1; var4 <= 9; ++var4) {
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 1, 3, var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 9, 3, var4, var3);
				}

				for(var4 = 1; var4 <= 9; ++var4) {
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, var4, 3, 1, var3);
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, var4, 3, 9, var3);
				}

				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 5, 1, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 5, 1, 6, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 5, 3, 4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 5, 3, 6, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 4, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 6, 1, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 4, 3, 5, var3);
				this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 6, 3, 5, var3);

				for(var4 = 1; var4 <= 3; ++var4) {
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 4, var4, 4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 6, var4, 4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 4, var4, 6, var3);
					this.placeBlockAtCurrentPosition(var1, Block.cobblestone.blockID, 0, 6, var4, 6, var3);
				}

				this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 5, 3, 5, var3);

				for(var4 = 2; var4 <= 8; ++var4) {
					this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 2, 3, var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 3, 3, var4, var3);
					if(var4 <= 3 || var4 >= 7) {
						this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 4, 3, var4, var3);
						this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 5, 3, var4, var3);
						this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 6, 3, var4, var3);
					}

					this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 7, 3, var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, 8, 3, var4, var3);
				}

				this.placeBlockAtCurrentPosition(var1, Block.ladder.blockID, this.getMetadataWithOffset(Block.ladder.blockID, 4), 9, 1, 3, var3);
				this.placeBlockAtCurrentPosition(var1, Block.ladder.blockID, this.getMetadataWithOffset(Block.ladder.blockID, 4), 9, 2, 3, var3);
				this.placeBlockAtCurrentPosition(var1, Block.ladder.blockID, this.getMetadataWithOffset(Block.ladder.blockID, 4), 9, 3, 3, var3);
				this.createTreasureChestAtCurrentPosition(var1, var3, var2, 3, 4, 8, chestLoot, 1 + var2.nextInt(4));
			}

			return true;
		}
	}
}
