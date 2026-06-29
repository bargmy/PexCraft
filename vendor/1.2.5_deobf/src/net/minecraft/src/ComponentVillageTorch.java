package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentVillageTorch extends ComponentVillage {
	private int averageGroundLevel = -1;

	public ComponentVillageTorch(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
	}

	public static StructureBoundingBox findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5) {
		StructureBoundingBox var6 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, 0, 0, 0, 3, 4, 2, var5);
		return StructureComponent.findIntersecting(var0, var6) != null ? null : var6;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.averageGroundLevel < 0) {
			this.averageGroundLevel = this.getAverageGroundLevel(var1, var3);
			if(this.averageGroundLevel < 0) {
				return true;
			}

			this.boundingBox.offset(0, this.averageGroundLevel - this.boundingBox.maxY + 4 - 1, 0);
		}

		this.fillWithBlocks(var1, var3, 0, 0, 0, 2, 3, 1, 0, 0, false);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 0, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 1, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.fence.blockID, 0, 1, 2, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.cloth.blockID, 15, 1, 3, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 15, 0, 3, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 15, 1, 3, 1, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 15, 2, 3, 0, var3);
		this.placeBlockAtCurrentPosition(var1, Block.torchWood.blockID, 15, 1, 3, -1, var3);
		return true;
	}
}
