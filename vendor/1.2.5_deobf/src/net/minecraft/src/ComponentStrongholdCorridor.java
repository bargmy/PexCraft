package net.minecraft.src;

import java.util.List;
import java.util.Random;

public class ComponentStrongholdCorridor extends ComponentStronghold {
	private final int field_35052_a;

	public ComponentStrongholdCorridor(int var1, Random var2, StructureBoundingBox var3, int var4) {
		super(var1);
		this.coordBaseMode = var4;
		this.boundingBox = var3;
		this.field_35052_a = var4 != 2 && var4 != 0 ? var3.getXSize() : var3.getZSize();
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
	}

	public static StructureBoundingBox func_35051_a(List var0, Random var1, int var2, int var3, int var4, int var5) {
		StructureBoundingBox var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, -1, 0, 5, 5, 4, var5);
		StructureComponent var8 = StructureComponent.findIntersecting(var0, var7);
		if(var8 == null) {
			return null;
		} else {
			if(var8.getBoundingBox().minY == var7.minY) {
				for(int var9 = 3; var9 >= 1; --var9) {
					var7 = StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, -1, 0, 5, 5, var9 - 1, var5);
					if(!var8.getBoundingBox().intersectsWith(var7)) {
						return StructureBoundingBox.getComponentToAddBoundingBox(var2, var3, var4, -1, -1, 0, 5, 5, var9, var5);
					}
				}
			}

			return null;
		}
	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			for(int var4 = 0; var4 < this.field_35052_a; ++var4) {
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 0, 0, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 0, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 2, 0, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 0, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 4, 0, var4, var3);

				for(int var5 = 1; var5 <= 3; ++var5) {
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 0, var5, var4, var3);
					this.placeBlockAtCurrentPosition(var1, 0, 0, 1, var5, var4, var3);
					this.placeBlockAtCurrentPosition(var1, 0, 0, 2, var5, var4, var3);
					this.placeBlockAtCurrentPosition(var1, 0, 0, 3, var5, var4, var3);
					this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 4, var5, var4, var3);
				}

				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 0, 4, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 1, 4, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 2, 4, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 3, 4, var4, var3);
				this.placeBlockAtCurrentPosition(var1, Block.stoneBrick.blockID, 0, 4, 4, var4, var3);
			}

			return true;
		}
	}
}
