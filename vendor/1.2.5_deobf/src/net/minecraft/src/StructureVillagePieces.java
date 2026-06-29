package net.minecraft.src;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Random;

public class StructureVillagePieces {
	public static ArrayList getStructureVillageWeightedPieceList(Random var0, int var1) {
		ArrayList var2 = new ArrayList();
		var2.add(new StructureVillagePieceWeight(ComponentVillageHouse4_Garden.class, 4, MathHelper.getRandomIntegerInRange(var0, 2 + var1, 4 + var1 * 2)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageChurch.class, 20, MathHelper.getRandomIntegerInRange(var0, 0 + var1, 1 + var1)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageHouse1.class, 20, MathHelper.getRandomIntegerInRange(var0, 0 + var1, 2 + var1)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageWoodHut.class, 3, MathHelper.getRandomIntegerInRange(var0, 2 + var1, 5 + var1 * 3)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageHall.class, 15, MathHelper.getRandomIntegerInRange(var0, 0 + var1, 2 + var1)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageField.class, 3, MathHelper.getRandomIntegerInRange(var0, 1 + var1, 4 + var1)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageField2.class, 3, MathHelper.getRandomIntegerInRange(var0, 2 + var1, 4 + var1 * 2)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageHouse2.class, 15, MathHelper.getRandomIntegerInRange(var0, 0, 1 + var1)));
		var2.add(new StructureVillagePieceWeight(ComponentVillageHouse3.class, 8, MathHelper.getRandomIntegerInRange(var0, 0 + var1, 3 + var1 * 2)));
		Iterator var3 = var2.iterator();

		while(var3.hasNext()) {
			if(((StructureVillagePieceWeight)var3.next()).villagePiecesLimit == 0) {
				var3.remove();
			}
		}

		return var2;
	}

	private static int getAvailablePieceWeight(ArrayList var0) {
		boolean var1 = false;
		int var2 = 0;

		StructureVillagePieceWeight var4;
		for(Iterator var3 = var0.iterator(); var3.hasNext(); var2 += var4.villagePieceWeight) {
			var4 = (StructureVillagePieceWeight)var3.next();
			if(var4.villagePiecesLimit > 0 && var4.villagePiecesSpawned < var4.villagePiecesLimit) {
				var1 = true;
			}
		}

		return var1 ? var2 : -1;
	}

	private static ComponentVillage getVillageComponentFromWeightedPiece(StructureVillagePieceWeight var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		Class var8 = var0.villagePieceClass;
		Object var9 = null;
		if(var8 == ComponentVillageHouse4_Garden.class) {
			var9 = ComponentVillageHouse4_Garden.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageChurch.class) {
			var9 = ComponentVillageChurch.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageHouse1.class) {
			var9 = ComponentVillageHouse1.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageWoodHut.class) {
			var9 = ComponentVillageWoodHut.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageHall.class) {
			var9 = ComponentVillageHall.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageField.class) {
			var9 = ComponentVillageField.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageField2.class) {
			var9 = ComponentVillageField2.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageHouse2.class) {
			var9 = ComponentVillageHouse2.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		} else if(var8 == ComponentVillageHouse3.class) {
			var9 = ComponentVillageHouse3.findValidPlacement(var1, var2, var3, var4, var5, var6, var7);
		}

		return (ComponentVillage)var9;
	}

	private static ComponentVillage getNextVillageComponent(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		int var8 = getAvailablePieceWeight(var0.structureVillageWeightedPieceList);
		if(var8 <= 0) {
			return null;
		} else {
			int var9 = 0;

			while(var9 < 5) {
				++var9;
				int var10 = var2.nextInt(var8);
				Iterator var11 = var0.structureVillageWeightedPieceList.iterator();

				while(var11.hasNext()) {
					StructureVillagePieceWeight var12 = (StructureVillagePieceWeight)var11.next();
					var10 -= var12.villagePieceWeight;
					if(var10 < 0) {
						if(!var12.canSpawnMoreVillagePiecesOfType(var7) || var12 == var0.structVillagePieceWeight && var0.structureVillageWeightedPieceList.size() > 1) {
							break;
						}

						ComponentVillage var13 = getVillageComponentFromWeightedPiece(var12, var1, var2, var3, var4, var5, var6, var7);
						if(var13 != null) {
							++var12.villagePiecesSpawned;
							var0.structVillagePieceWeight = var12;
							if(!var12.canSpawnMoreVillagePieces()) {
								var0.structureVillageWeightedPieceList.remove(var12);
							}

							return var13;
						}
					}
				}
			}

			StructureBoundingBox var14 = ComponentVillageTorch.findValidPlacement(var1, var2, var3, var4, var5, var6);
			if(var14 != null) {
				return new ComponentVillageTorch(var7, var2, var14, var6);
			} else {
				return null;
			}
		}
	}

	private static StructureComponent getNextVillageStructureComponent(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		if(var7 > 50) {
			return null;
		} else if(Math.abs(var3 - var0.getBoundingBox().minX) <= 112 && Math.abs(var5 - var0.getBoundingBox().minZ) <= 112) {
			ComponentVillage var8 = getNextVillageComponent(var0, var1, var2, var3, var4, var5, var6, var7 + 1);
			if(var8 != null) {
				int var9 = (var8.boundingBox.minX + var8.boundingBox.maxX) / 2;
				int var10 = (var8.boundingBox.minZ + var8.boundingBox.maxZ) / 2;
				int var11 = var8.boundingBox.maxX - var8.boundingBox.minX;
				int var12 = var8.boundingBox.maxZ - var8.boundingBox.minZ;
				int var13 = var11 > var12 ? var11 : var12;
				if(var0.getWorldChunkManager().areBiomesViable(var9, var10, var13 / 2 + 4, MapGenVillage.villageSpawnBiomes)) {
					var1.add(var8);
					var0.field_35108_e.add(var8);
					return var8;
				}
			}

			return null;
		} else {
			return null;
		}
	}

	private static StructureComponent getNextComponentVillagePath(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		if(var7 > 3 + var0.terrainType) {
			return null;
		} else if(Math.abs(var3 - var0.getBoundingBox().minX) <= 112 && Math.abs(var5 - var0.getBoundingBox().minZ) <= 112) {
			StructureBoundingBox var8 = ComponentVillagePathGen.func_35087_a(var0, var1, var2, var3, var4, var5, var6);
			if(var8 != null && var8.minY > 10) {
				ComponentVillagePathGen var9 = new ComponentVillagePathGen(var7, var2, var8, var6);
				int var10 = (var9.boundingBox.minX + var9.boundingBox.maxX) / 2;
				int var11 = (var9.boundingBox.minZ + var9.boundingBox.maxZ) / 2;
				int var12 = var9.boundingBox.maxX - var9.boundingBox.minX;
				int var13 = var9.boundingBox.maxZ - var9.boundingBox.minZ;
				int var14 = var12 > var13 ? var12 : var13;
				if(var0.getWorldChunkManager().areBiomesViable(var10, var11, var14 / 2 + 4, MapGenVillage.villageSpawnBiomes)) {
					var1.add(var9);
					var0.field_35106_f.add(var9);
					return var9;
				}
			}

			return null;
		} else {
			return null;
		}
	}

	static StructureComponent getNextStructureComponent(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		return getNextVillageStructureComponent(var0, var1, var2, var3, var4, var5, var6, var7);
	}

	static StructureComponent getNextStructureComponentVillagePath(ComponentVillageStartPiece var0, List var1, Random var2, int var3, int var4, int var5, int var6, int var7) {
		return getNextComponentVillagePath(var0, var1, var2, var3, var4, var5, var6, var7);
	}
}
