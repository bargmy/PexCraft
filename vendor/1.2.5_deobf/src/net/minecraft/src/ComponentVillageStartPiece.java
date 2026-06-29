package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class ComponentVillageStartPiece extends ComponentVillageWell {
	public WorldChunkManager worldChunkMngr;
	public int terrainType;
	public StructureVillagePieceWeight structVillagePieceWeight;
	public ArrayList structureVillageWeightedPieceList;
	public ArrayList field_35108_e = new ArrayList();
	public ArrayList field_35106_f = new ArrayList();

	public ComponentVillageStartPiece(WorldChunkManager var1, int var2, Random var3, int var4, int var5, ArrayList var6, int var7) {
		super(0, var3, var4, var5);
		this.worldChunkMngr = var1;
		this.structureVillageWeightedPieceList = var6;
		this.terrainType = var7;
	}

	public WorldChunkManager getWorldChunkManager() {
		return this.worldChunkMngr;
	}
}
