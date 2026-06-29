package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentNetherBridgeCorridor extends ComponentNetherBridgePiece {
	public ComponentNetherBridgeCorridor(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		this.getNextComponentX((ComponentNetherBridgeStartPiece)var1, var2, var3, 0, 1, true);
	}

	public static ComponentNetherBridgeCorridor createValidComponent(List var0, Random var1, int var2, int var3, int var4, int var5, int var6) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, 0, 0, 5, 7, 5, var5);
		return isAboveGround(var7) && StructureComponent.findIntersecting(var0, var7) == null ? new ComponentNetherBridgeCorridor(var6, var1, var7, var5) : null;
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		this.fillWithBlocks(var1, var3, 0, 0, 0, 4, 1, 4, Block.netherBrick.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 2, 0, 4, 5, 4, 0, 0, false);
		this.fillWithBlocks(var1, var3, 4, 2, 0, 4, 5, 4, Block.netherBrick.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 4, 3, 1, 4, 4, 1, Block.netherFence.blockID, Block.netherFence.blockID, false);
		this.fillWithBlocks(var1, var3, 4, 3, 3, 4, 4, 3, Block.netherFence.blockID, Block.netherFence.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 2, 0, 0, 5, 0, Block.netherBrick.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 2, 4, 3, 5, 4, Block.netherBrick.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 1, 3, 4, 1, 4, 4, Block.netherFence.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 3, 3, 4, 3, 4, 4, Block.netherFence.blockID, Block.netherBrick.blockID, false);
		this.fillWithBlocks(var1, var3, 0, 6, 0, 4, 6, 4, Block.netherBrick.blockID, Block.netherBrick.blockID, false);

		for(int var4 = 0; var4 <= 4; ++var4) {
			for(int var5 = 0; var5 <= 4; ++var5) {
				this.fillCurrentPositionBlocksDownwards(var1, Block.netherBrick.blockID, 0, var4, -1, var5, var3);
			}
		}

		return true;
	}
}
