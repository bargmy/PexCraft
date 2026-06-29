package net.minecraft.src;

import java.util.List;
import java.util.Random;

abstract class ComponentStronghold extends StructureComponent {
	protected ComponentStronghold(int var1) {
		super(var1);
	}

	protected void placeDoor(World var1, Random var2, StructureBoundingBox var3, EnumDoor var4, int var5, int var6, int var7) {
		switch(EnumDoorHelper.doorEnum[var4.ordinal()]) {
		case 1:
		default:
			this.fillWithBlocks(var1, var3, var5, var6, var7, var5 + 3 - 1, var6 + 3 - 1, var7, 0, 0, false);
			break;
		case 2:
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 1, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.doorWood.blockID, 0, var5 + 1, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.doorWood.blockID, 8, var5 + 1, var6 + 1, var7, var3);
			break;
		case 3:
			this.placeBlockAtCurrentPosition(var1, 0, 0, var5 + 1, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, 0, 0, var5 + 1, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5 + 1, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5 + 2, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5 + 2, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.fenceIron.blockID, 0, var5 + 2, var6, var7, var3);
			break;
		case 4:
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 1, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6 + 2, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, var5 + 2, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.doorSteel.blockID, 0, var5 + 1, var6, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.doorSteel.blockID, 8, var5 + 1, var6 + 1, var7, var3);
			this.placeBlockAtCurrentPosition(var1, Block.button.blockID, this.getMetadataWithOffset(Block.button.blockID, 4), var5 + 2, var6 + 1, var7 + 1, var3);
			this.placeBlockAtCurrentPosition(var1, Block.button.blockID, this.getMetadataWithOffset(Block.button.blockID, 3), var5 + 2, var6 + 1, var7 - 1, var3);
		}

	}

	protected EnumDoor getRandomDoor(Random var1) {
		int var2 = var1.nextInt(5);
		switch(var2) {
		case 0:
		case 1:
		default:
			return EnumDoor.OPENING;
		case 2:
			return EnumDoor.WOOD_DOOR;
		case 3:
			return EnumDoor.GRATES;
		case 4:
			return EnumDoor.IRON_DOOR;
		}
	}

	protected StructureComponent getNextComponentNormal(ComponentStrongholdStairs2 var1, List var2, Random var3, int var4, int var5) {
		switch(this.coordBaseMode) {
		case 0:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var4, this.boundingBox.minY + var5, this.boundingBox.maxZ + 1, this.coordBaseMode, this.getComponentType());
		case 1:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var5, this.boundingBox.minZ + var4, this.coordBaseMode, this.getComponentType());
		case 2:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var4, this.boundingBox.minY + var5, this.boundingBox.minZ - 1, this.coordBaseMode, this.getComponentType());
		case 3:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var5, this.boundingBox.minZ + var4, this.coordBaseMode, this.getComponentType());
		default:
			return null;
		}
	}

	protected StructureComponent getNextComponentX(ComponentStrongholdStairs2 var1, List var2, Random var3, int var4, int var5) {
		switch(this.coordBaseMode) {
		case 0:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 1, this.getComponentType());
		case 1:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.minZ - 1, 2, this.getComponentType());
		case 2:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 1, this.getComponentType());
		case 3:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.minZ - 1, 2, this.getComponentType());
		default:
			return null;
		}
	}

	protected StructureComponent getNextComponentZ(ComponentStrongholdStairs2 var1, List var2, Random var3, int var4, int var5) {
		switch(this.coordBaseMode) {
		case 0:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 3, this.getComponentType());
		case 1:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.maxZ + 1, 0, this.getComponentType());
		case 2:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 3, this.getComponentType());
		case 3:
			return StructureStrongholdPieces.getNextValidComponentAccess(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.maxZ + 1, 0, this.getComponentType());
		default:
			return null;
		}
	}

	protected static boolean canStrongholdGoDeeper(StructureBoundingBox var0) {
		return var0 != null && var0.minY > 10;
	}
}
