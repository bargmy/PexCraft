package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentMineshaftCorridor extends StructureComponent {
	private final boolean hasRails;
	private final boolean hasSpiders;
	private boolean spawnerPlaced;
	private int sectionCount;

	public ComponentMineshaftCorridor(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
		this.hasRails = var2.nextInt(3) == 0;
		this.hasSpiders = !this.hasRails && var2.nextInt(23) == 0;
		if(this.coordBaseMode != 2 && this.coordBaseMode != 0) {
			this.sectionCount = var3.getXSize() / 5;
		} else {
			this.sectionCount = var3.getZSize() / 5;
		}

	}

	public static StructureBoundingBox findValidPlacement(List var0, Random var1, int var2, int var3, int var4, int var5) {
		StructureBoundingBox var6 = new StructureBoundingBox(var2, var3, var4, var2, var3 + 2, var4);

		int var7;
		for(var7 = var1.nextInt(3) + 2; var7 > 0; --var7) {
			int var8 = var7 * 5;
			switch(var5) {
			case 0:
				var6.maxX = var2 + 2;
				var6.maxZ = var4 + (var8 - 1);
				break;
			case 1:
				var6.minX = var2 - (var8 - 1);
				var6.maxZ = var4 + 2;
				break;
			case 2:
				var6.maxX = var2 + 2;
				var6.minZ = var4 - (var8 - 1);
				break;
			case 3:
				var6.maxX = var2 + (var8 - 1);
				var6.maxZ = var4 + 2;
			}

			if(StructureComponent.findIntersecting(var0, var6) == null) {
				break;
			}
		}

		return var7 > 0 ? var6 : null;
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		int var4 = this.getComponentType();
		int var5 = var3.nextInt(4);
		switch(this.coordBaseMode) {
		case 0:
			if(var5 <= 1) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.maxZ + 1, this.coordBaseMode, var4);
			} else if(var5 == 2) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.maxZ - 3, 1, var4);
			} else {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.maxZ - 3, 3, var4);
			}
			break;
		case 1:
			if(var5 <= 1) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ, this.coordBaseMode, var4);
			} else if(var5 == 2) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ - 1, 2, var4);
			} else {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.maxZ + 1, 0, var4);
			}
			break;
		case 2:
			if(var5 <= 1) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ - 1, this.coordBaseMode, var4);
			} else if(var5 == 2) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ, 1, var4);
			} else {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ, 3, var4);
			}
			break;
		case 3:
			if(var5 <= 1) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ, this.coordBaseMode, var4);
			} else if(var5 == 2) {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX - 3, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.minZ - 1, 2, var4);
			} else {
				StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX - 3, this.boundingBox.minY - 1 + var3.nextInt(3), this.boundingBox.maxZ + 1, 0, var4);
			}
		}

		if(var4 < 8) {
			int var6;
			int var7;
			if(this.coordBaseMode != 2 && this.coordBaseMode != 0) {
				for(var6 = this.boundingBox.minX + 3; var6 + 3 <= this.boundingBox.maxX; var6 += 5) {
					var7 = var3.nextInt(5);
					if(var7 == 0) {
						StructureMineshaftPieces.getNextComponent(var1, var2, var3, var6, this.boundingBox.minY, this.boundingBox.minZ - 1, 2, var4 + 1);
					} else if(var7 == 1) {
						StructureMineshaftPieces.getNextComponent(var1, var2, var3, var6, this.boundingBox.minY, this.boundingBox.maxZ + 1, 0, var4 + 1);
					}
				}
			} else {
				for(var6 = this.boundingBox.minZ + 3; var6 + 3 <= this.boundingBox.maxZ; var6 += 5) {
					var7 = var3.nextInt(5);
					if(var7 == 0) {
						StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY, var6, 1, var4 + 1);
					} else if(var7 == 1) {
						StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY, var6, 3, var4 + 1);
					}
				}
			}
		}

	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			int var8 = this.sectionCount * 5 - 1;
			this.fillWithBlocks(var1, var3, 0, 0, 0, 2, 1, var8, 0, 0, false);
			this.randomlyFillWithBlocks(var1, var3, var2, 0.8F, 0, 2, 0, 2, 2, var8, 0, 0, false);
			if(this.hasSpiders) {
				this.randomlyFillWithBlocks(var1, var3, var2, 0.6F, 0, 0, 0, 2, 1, var8, Block.web.blockID, 0, false);
			}

			int var9;
			int var10;
			int var11;
			for(var9 = 0; var9 < this.sectionCount; ++var9) {
				var10 = 2 + var9 * 5;
				this.fillWithBlocks(var1, var3, 0, 0, var10, 0, 1, var10, Block.fence.blockID, 0, false);
				this.fillWithBlocks(var1, var3, 2, 0, var10, 2, 1, var10, Block.fence.blockID, 0, false);
				if(var2.nextInt(4) != 0) {
					this.fillWithBlocks(var1, var3, 0, 2, var10, 2, 2, var10, Block.planks.blockID, 0, false);
				} else {
					this.fillWithBlocks(var1, var3, 0, 2, var10, 0, 2, var10, Block.planks.blockID, 0, false);
					this.fillWithBlocks(var1, var3, 2, 2, var10, 2, 2, var10, Block.planks.blockID, 0, false);
				}

				this.randomlyPlaceBlock(var1, var3, var2, 0.1F, 0, 2, var10 - 1, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.1F, 2, 2, var10 - 1, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.1F, 0, 2, var10 + 1, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.1F, 2, 2, var10 + 1, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 0, 2, var10 - 2, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 2, 2, var10 - 2, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 0, 2, var10 + 2, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 2, 2, var10 + 2, Block.web.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 1, 2, var10 - 1, Block.torchWood.blockID, 0);
				this.randomlyPlaceBlock(var1, var3, var2, 0.05F, 1, 2, var10 + 1, Block.torchWood.blockID, 0);
				if(var2.nextInt(100) == 0) {
					this.createTreasureChestAtCurrentPosition(var1, var3, var2, 2, 0, var10 - 1, StructureMineshaftPieces.getTreasurePieces(), 3 + var2.nextInt(4));
				}

				if(var2.nextInt(100) == 0) {
					this.createTreasureChestAtCurrentPosition(var1, var3, var2, 0, 0, var10 + 1, StructureMineshaftPieces.getTreasurePieces(), 3 + var2.nextInt(4));
				}

				if(this.hasSpiders && !this.spawnerPlaced) {
					var11 = this.getYWithOffset(0);
					int var12 = var10 - 1 + var2.nextInt(3);
					int var13 = this.getXWithOffset(1, var12);
					var12 = this.getZWithOffset(1, var12);
					if(var3.isVecInside(var13, var11, var12)) {
						this.spawnerPlaced = true;
						var1.setBlockWithNotify(var13, var11, var12, Block.mobSpawner.blockID);
						TileEntityMobSpawner var14 = (TileEntityMobSpawner)var1.getBlockTileEntity(var13, var11, var12);
						if(var14 != null) {
							var14.setMobID("CaveSpider");
						}
					}
				}
			}

			for(var9 = 0; var9 <= 2; ++var9) {
				for(var10 = 0; var10 <= var8; ++var10) {
					var11 = this.getBlockIdAtCurrentPosition(var1, var9, -1, var10, var3);
					if(var11 == 0) {
						this.placeBlockAtCurrentPosition(var1, Block.planks.blockID, 0, var9, -1, var10, var3);
					}
				}
			}

			if(this.hasRails) {
				for(var9 = 0; var9 <= var8; ++var9) {
					var10 = this.getBlockIdAtCurrentPosition(var1, 1, -1, var9, var3);
					if(var10 > 0 && Block.opaqueCubeLookup[var10]) {
						this.randomlyPlaceBlock(var1, var3, var2, 0.7F, 1, 0, var9, Block.rail.blockID, this.getMetadataWithOffset(Block.rail.blockID, 0));
					}
				}
			}

			return true;
		}
	}
}
