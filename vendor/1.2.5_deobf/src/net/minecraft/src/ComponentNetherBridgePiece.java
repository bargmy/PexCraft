package net.minecraft.src;

import java.util.Iterator;
import java.util.List;
import java.util.Random;

abstract class ComponentNetherBridgePiece extends StructureComponent {
	protected ComponentNetherBridgePiece(int var1) {
		super(var1);
	}

	private int getTotalWeight(List var1) {
		boolean var2 = false;
		int var3 = 0;

		StructureNetherBridgePieceWeight var5;
		for(Iterator var4 = var1.iterator(); var4.hasNext(); var3 += var5.field_40697_b) {
			var5 = (StructureNetherBridgePieceWeight)var4.next();
			if(var5.field_40695_d > 0 && var5.field_40698_c < var5.field_40695_d) {
				var2 = true;
			}
		}

		return var2 ? var3 : -1;
	}

	private ComponentNetherBridgePiece getNextComponent(ComponentNetherBridgeStartPiece var1, List var2, List var3, Random var4, int var5, int var6, int var7, int var8, int var9) {
		int var10 = this.getTotalWeight(var2);
		boolean var11 = var10 > 0 && var9 <= 30;
		int var12 = 0;

		while(var12 < 5 && var11) {
			++var12;
			int var13 = var4.nextInt(var10);
			Iterator var14 = var2.iterator();

			while(var14.hasNext()) {
				StructureNetherBridgePieceWeight var15 = (StructureNetherBridgePieceWeight)var14.next();
				var13 -= var15.field_40697_b;
				if(var13 < 0) {
					if(!var15.func_40693_a(var9) || var15 == var1.field_40037_a && !var15.field_40696_e) {
						break;
					}

					ComponentNetherBridgePiece var16 = StructureNetherBridgePieces.createNextComponent(var15, var3, var4, var5, var6, var7, var8, var9);
					if(var16 != null) {
						++var15.field_40698_c;
						var1.field_40037_a = var15;
						if(!var15.func_40694_a()) {
							var2.remove(var15);
						}

						return var16;
					}
				}
			}
		}

		ComponentNetherBridgeEnd var17 = ComponentNetherBridgeEnd.func_40023_a(var3, var4, var5, var6, var7, var8, var9);
		return var17;
	}

	private StructureComponent getNextComponent(ComponentNetherBridgeStartPiece var1, List var2, Random var3, int var4, int var5, int var6, int var7, int var8, boolean var9) {
		if(Math.abs(var4 - var1.getBoundingBox().minX) <= 112 && Math.abs(var6 - var1.getBoundingBox().minZ) <= 112) {
			List var12 = var1.field_40035_b;
			if(var9) {
				var12 = var1.field_40036_c;
			}

			ComponentNetherBridgePiece var11 = this.getNextComponent(var1, var12, var2, var3, var4, var5, var6, var7, var8 + 1);
			if(var11 != null) {
				var2.add(var11);
				var1.field_40034_d.add(var11);
			}

			return var11;
		} else {
			ComponentNetherBridgeEnd var10 = ComponentNetherBridgeEnd.func_40023_a(var2, var3, var4, var5, var6, var7, var8);
			return var10;
		}
	}

	protected StructureComponent getNextComponentNormal(ComponentNetherBridgeStartPiece var1, List var2, Random var3, int var4, int var5, boolean var6) {
		switch(this.coordBaseMode) {
		case 0:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var4, this.boundingBox.minY + var5, this.boundingBox.maxZ + 1, this.coordBaseMode, this.getComponentType(), var6);
		case 1:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var5, this.boundingBox.minZ + var4, this.coordBaseMode, this.getComponentType(), var6);
		case 2:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var4, this.boundingBox.minY + var5, this.boundingBox.minZ - 1, this.coordBaseMode, this.getComponentType(), var6);
		case 3:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var5, this.boundingBox.minZ + var4, this.coordBaseMode, this.getComponentType(), var6);
		default:
			return null;
		}
	}

	protected StructureComponent getNextComponentX(ComponentNetherBridgeStartPiece var1, List var2, Random var3, int var4, int var5, boolean var6) {
		switch(this.coordBaseMode) {
		case 0:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 1, this.getComponentType(), var6);
		case 1:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.minZ - 1, 2, this.getComponentType(), var6);
		case 2:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 1, this.getComponentType(), var6);
		case 3:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.minZ - 1, 2, this.getComponentType(), var6);
		default:
			return null;
		}
	}

	protected StructureComponent getNextComponentZ(ComponentNetherBridgeStartPiece var1, List var2, Random var3, int var4, int var5, boolean var6) {
		switch(this.coordBaseMode) {
		case 0:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 3, this.getComponentType(), var6);
		case 1:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.maxZ + 1, 0, this.getComponentType(), var6);
		case 2:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var4, this.boundingBox.minZ + var5, 3, this.getComponentType(), var6);
		case 3:
			return this.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var4, this.boundingBox.maxZ + 1, 0, this.getComponentType(), var6);
		default:
			return null;
		}
	}

	protected static boolean isAboveGround(StructureBoundingBox var0) {
		return var0 != null && var0.minY > 10;
	}
}
