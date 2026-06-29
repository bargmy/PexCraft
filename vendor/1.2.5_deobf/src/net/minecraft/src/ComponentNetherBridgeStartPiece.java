package net.minecraft.src;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class ComponentNetherBridgeStartPiece extends ComponentNetherBridgeCrossing3 {
	public StructureNetherBridgePieceWeight field_40037_a;
	public List field_40035_b = new ArrayList();
	public List field_40036_c;
	public ArrayList field_40034_d = new ArrayList();

	public ComponentNetherBridgeStartPiece(Random var1, int var2, int var3) {
		super(var1, var2, var3);
		StructureNetherBridgePieceWeight[] var4 = StructureNetherBridgePieces.getPrimaryComponents();
		int var5 = var4.length;

		int var6;
		StructureNetherBridgePieceWeight var7;
		for(var6 = 0; var6 < var5; ++var6) {
			var7 = var4[var6];
			var7.field_40698_c = 0;
			this.field_40035_b.add(var7);
		}

		this.field_40036_c = new ArrayList();
		var4 = StructureNetherBridgePieces.getSecondaryComponents();
		var5 = var4.length;

		for(var6 = 0; var6 < var5; ++var6) {
			var7 = var4[var6];
			var7.field_40698_c = 0;
			this.field_40036_c.add(var7);
		}

	}
}
