package net.minecraft.src;

import java.util.Random;

class StructureStrongholdStones extends StructurePieceBlockSelector {
	private StructureStrongholdStones() {
	}

	public void selectBlocks(Random var1, int var2, int var3, int var4, boolean var5) {
		if(!var5) {
			this.selectedBlockId = 0;
			this.selectedBlockMetaData = 0;
		} else {
			this.selectedBlockId = Block.stoneBrick.blockID;
			float var6 = var1.nextFloat();
			if(var6 < 0.2F) {
				this.selectedBlockMetaData = 2;
			} else if(var6 < 0.5F) {
				this.selectedBlockMetaData = 1;
			} else if(var6 < 0.55F) {
				this.selectedBlockId = Block.silverfish.blockID;
				this.selectedBlockMetaData = 2;
			} else {
				this.selectedBlockMetaData = 0;
			}
		}

	}

	StructureStrongholdStones(StructureStrongholdPieceWeight2 var1) {
		this();
	}
}
