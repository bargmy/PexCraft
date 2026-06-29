package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentMineshaftCross extends StructureComponent {
	private final int corridorDirection;
	private final boolean isMultipleFloors;

	public ComponentMineshaftCross(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.corridorDirection = var4;
		this.boundingBox = var3;
		this.isMultipleFloors = var3.getYSize() > 3;
	}

	public static StructureBoundingBox findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5) {
		StructureBoundingBox var6 = new StructureBoundingBox(var2, var3, var4, var2, var3 + 2, var4);
		if(var1.nextInt(4) == 0) {
			var6.maxY += 4;
		}

		switch(var5) {
		case 0:
			var6.minX = var2 - 1;
			var6.maxX = var2 + 3;
			var6.maxZ = var4 + 4;
			break;
		case 1:
			var6.minX = var2 - 4;
			var6.minZ = var4 - 1;
			var6.maxZ = var4 + 3;
			break;
		case 2:
			var6.minX = var2 - 1;
			var6.maxX = var2 + 3;
			var6.minZ = var4 - 4;
			break;
		case 3:
			var6.maxX = var2 + 4;
			var6.minZ = var4 - 1;
			var6.maxZ = var4 + 3;
		}

		return StructureComponent.findIntersecting(var0, var6) != null ? null : var6;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		int var4 = this.getComponentType();
		switch(this.corridorDirection) {
		case 0:
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 1, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 3, var4);
			break;
		case 1:
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 1, var4);
			break;
		case 2:
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 1, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 3, var4);
			break;
		case 3:
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, var4);
			StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, this.boundingBox.minZ + 1, 3, var4);
		}

		if(this.isMultipleFloors) {
			if(var3.nextBoolean()) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY + 3 + 1, this.boundingBox.minZ - 1, 2, var4);
			}

			if(var3.nextBoolean()) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + 3 + 1, this.boundingBox.minZ + 1, 1, var4);
			}

			if(var3.nextBoolean()) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + 3 + 1, this.boundingBox.minZ + 1, 3, var4);
			}

			if(var3.nextBoolean()) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + 1, this.boundingBox.minY + 3 + 1, this.boundingBox.maxZ + 1, 0, var4);
			}
		}

	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			if(this.isMultipleFloors) {
				this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ, this.boundingBox.maxX - 1, this.boundingBox.minY + 3 - 1, this.boundingBox.maxZ, 0, 0, false);
				this.fillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.minY, this.boundingBox.minZ + 1, this.boundingBox.maxX, this.boundingBox.minY + 3 - 1, this.boundingBox.maxZ - 1, 0, 0, false);
				this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.maxY - 2, this.boundingBox.minZ, this.boundingBox.maxX - 1, this.boundingBox.maxY, this.boundingBox.maxZ, 0, 0, false);
				this.fillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.maxY - 2, this.boundingBox.minZ + 1, this.boundingBox.maxX, this.boundingBox.maxY, this.boundingBox.maxZ - 1, 0, 0, false);
				this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.minY + 3, this.boundingBox.minZ + 1, this.boundingBox.maxX - 1, this.boundingBox.minY + 3, this.boundingBox.maxZ - 1, 0, 0, false);
			} else {
				this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ, this.boundingBox.maxX - 1, this.boundingBox.maxY, this.boundingBox.maxZ, 0, 0, false);
				this.fillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.minY, this.boundingBox.minZ + 1, this.boundingBox.maxX, this.boundingBox.maxY, this.boundingBox.maxZ - 1, 0, 0, false);
			}

			this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.minZ + 1, this.boundingBox.minX + 1, this.boundingBox.maxY, this.boundingBox.minZ + 1, Block.planks.blockID, 0, false);
			this.fillWithBlocks(var1, var3, this.boundingBox.minX + 1, this.boundingBox.minY, this.boundingBox.maxZ - 1, this.boundingBox.minX + 1, this.boundingBox.maxY, this.boundingBox.maxZ - 1, Block.planks.blockID, 0, false);
			this.fillWithBlocks(var1, var3, this.boundingBox.maxX - 1, this.boundingBox.minY, this.boundingBox.minZ + 1, this.boundingBox.maxX - 1, this.boundingBox.maxY, this.boundingBox.minZ + 1, Block.planks.blockID, 0, false);
			this.fillWithBlocks(var1, var3, this.boundingBox.maxX - 1, this.boundingBox.minY, this.boundingBox.maxZ - 1, this.boundingBox.maxX - 1, this.boundingBox.maxY, this.boundingBox.maxZ - 1, Block.planks.blockID, 0, false);

			for(int var4 = this.boundingBox.minX; var4 <= this.boundingBox.maxX; ++var4) {
				for(int var5 = this.boundingBox.minZ; var5 <= this.boundingBox.maxZ; ++var5) {
					int var6 = this.getBlockIdAtCurrentPosition(var1, var4, this.boundingBox.minY - 1, var5, var3);
					if(var6 == 0) {
						this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, var4, this.boundingBox.minY - 1, var5, var3);
					}
				}
			}

			return true;
		}
	}
}
