package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentStrongholdCrossing extends ComponentStronghold {
	protected final EnumDoor doorType;
	private boolean field_35042_b;
	private boolean field_35043_c;
	private boolean field_35040_d;
	private boolean field_35041_e;

	public ComponentStrongholdCrossing(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.doorType = this.getRandomDoor(var2);
		this.boundingBox = var3;
		this.field_35042_b = var2.nextBoolean();
		this.field_35043_c = var2.nextBoolean();
		this.field_35040_d = var2.nextBoolean();
		this.field_35041_e = var2.nextInt(3) > 0;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		int var4 = 3;
		int var5 = 5;
		if(this.coordBaseMode == 1 || this.coordBaseMode == 2) {
			var4 = 8 - var4;
			var5 = 8 - var5;
		}

		this.getNextComponentNormal((ComponentStrongholdStairs2)var1, var2, var3, 5, 1);
		if(this.field_35042_b) {
			this.getNextComponentX((ComponentStrongholdStairs2)var1, var2, var3, var4, 1);
		}

		if(this.field_35043_c) {
			this.getNextComponentX((ComponentStrongholdStairs2)var1, var2, var3, var5, 7);
		}

		if(this.field_35040_d) {
			this.getNextComponentZ((ComponentStrongholdStairs2)var1, var2, var3, var4, 1);
		}

		if(this.field_35041_e) {
			this.getNextComponentZ((ComponentStrongholdStairs2)var1, var2, var3, var5, 7);
		}

	}

	public static ComponentStrongholdCrossing findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -4, -3, 0, 10, 9, 11, var5);
		return canStrongholdGoDeeper(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentStrongholdCrossing(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			this.fillWithRandomizedBlocks(var1, var3, 0, 0, 0, 9, 8, 10, true, var2, StructureStrongholdPieces.getStrongholdStones());
			this.placeDoor(var1, var2, var3, this.doorType, 4, 3, 0);
			if(this.field_35042_b) {
				this.fillWithBlocks(var1, var3, 0, 3, 1, 0, 5, 3, 0, 0, false);
			}

			if(this.field_35040_d) {
				this.fillWithBlocks(var1, var3, 9, 3, 1, 9, 5, 3, 0, 0, false);
			}

			if(this.field_35043_c) {
				this.fillWithBlocks(var1, var3, 0, 5, 7, 0, 7, 9, 0, 0, false);
			}

			if(this.field_35041_e) {
				this.fillWithBlocks(var1, var3, 9, 5, 7, 9, 7, 9, 0, 0, false);
			}

			this.fillWithBlocks(var1, var3, 5, 1, 10, 7, 3, 10, 0, 0, false);
			this.fillWithRandomizedBlocks(var1, var3, 1, 2, 1, 8, 2, 6, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithRandomizedBlocks(var1, var3, 4, 1, 5, 4, 4, 9, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithRandomizedBlocks(var1, var3, 8, 1, 5, 8, 4, 9, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithRandomizedBlocks(var1, var3, 1, 4, 7, 3, 4, 9, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithRandomizedBlocks(var1, var3, 1, 3, 5, 3, 3, 6, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithBlocks(var1, var3, 1, 3, 4, 3, 3, 4, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithBlocks(var1, var3, 1, 4, 6, 3, 4, 6, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithRandomizedBlocks(var1, var3, 5, 1, 7, 7, 1, 8, false, var2, StructureStrongholdPieces.getStrongholdStones());
			this.fillWithBlocks(var1, var3, 5, 1, 9, 7, 1, 9, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithBlocks(var1, var3, 5, 2, 7, 7, 2, 7, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithBlocks(var1, var3, 4, 5, 7, 4, 5, 9, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithBlocks(var1, var3, 8, 5, 7, 8, 5, 9, Block.stairSingle.blockID, Block.stairSingle.blockID, false);
			this.fillWithBlocks(var1, var3, 5, 5, 7, 7, 5, 9, Block.stairDouble.blockID, Block.stairDouble.blockID, false);
			this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 0, 6, 5, 6, var3);
			return true;
		}
	}
}
