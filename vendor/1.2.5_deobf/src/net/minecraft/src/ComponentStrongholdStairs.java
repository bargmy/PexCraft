package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentStrongholdStairs extends ComponentStronghold {
	private final boolean field_35036_a;
	private final EnumDoor doorType;

	public ComponentStrongholdStairs(int var1, Random var2, int var3, int var4) {
		super(var1);
		this.field_35036_a = true;
		this.coordBaseMode = var2.nextInt(4);
		this.doorType = EnumDoor.OPENING;
		switch(this.coordBaseMode) {
		case 0:
		case 2:
			this.boundingBox = new StructureBoundingBox(var3, 64, var4, var3 + 5 - 1, 74, var4 + 5 - 1);
			break;
		default:
			this.boundingBox = new StructureBoundingBox(var3, 64, var4, var3 + 5 - 1, 74, var4 + 5 - 1);
		}

	}

	public ComponentStrongholdStairs(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.field_35036_a = false;
		this.coordBaseMode = var4;
		this.doorType = this.getRandomDoor(var2);
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		if(this.field_35036_a) {
			StructureStrongholdPieces.setComponentType(ComponentStrongholdCrossing.class);
		}

		this.getNextComponentNormal((ComponentStrongholdStairs2)var1, var2, var3, 1, 1);
	}

	public static ComponentStrongholdStairs getStrongholdStairsComponent(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, -7, 0, 5, 11, 5, var5);
		return canStrongholdGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentStrongholdStairs(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			if(this.field_35036_a) {
			}

			this.fillWithRandomizedBlocks(var1, var3, 0, 0, 0, 4, 10, 4, true, var2, StructureStrongholdPieces.getStrongholdStones());
			this.placeDoor(var1, var2, var3, this.doorType, 1, 7, 0);
			this.placeDoor(var1, var2, var3, EnumDoor.OPENING, 1, 1, 4);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 2, 6, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 5, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 1, 6, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 5, 2, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 4, 3, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 1, 5, 3, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 2, 4, 3, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 3, 3, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 3, 4, 3, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 3, 2, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 2, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 3, 3, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 2, 2, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 1, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 1, 2, 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 1, 2, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stairSingle.blockID, 0, 1, 1, 3, var3);
			return true;
		}
	}
}
