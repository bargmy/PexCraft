package net.minecraft.src;

import java.util.ArrayList;
import java.util.Random;

public class ComponentStrongholdStairs2 extends ComponentStrongholdStairs {
	public StructureStrongholdPieceWeight field_35038_a;
	public ComponentStrongholdPortalRoom portalRoom;
	public ArrayList field_35037_b = new ArrayList();

	public ComponentStrongholdStairs2(int var1, Random var2, int var3, int var4) {
		super(0, var2, var3, var4);
	}

	public ChunkPosition getCenter() {
		return this.portalRoom != null ? this.portalRoom.getCenter() : super.getCenter();
	}
}
