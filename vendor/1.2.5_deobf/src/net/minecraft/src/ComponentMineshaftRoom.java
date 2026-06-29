package net.minecraft.src;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

public class ComponentMineshaftRoom extends StructureComponent {
	private LinkedList chidStructures = new LinkedList();

	public ComponentMineshaftRoom(int var1, Random var2, int var3, int var4) {
		super(var1);
		this.boundingBox = new StructureBoundingBox(var3, 50, var4, var3 + 7 + var2.nextInt(6), 54 + var2.nextInt(6), var4 + 7 + var2.nextInt(6));
	}

	public void buildComponent(StructureComponent var1, List var2, Random var3) {
		int var4 = this.getComponentType();
		int var6 = this.boundingBox.getYSize() - 3 - 1;
		if(var6 <= 0) {
			var6 = 1;
		}

		int var5;
		StructureComponent var7;
		StructureBoundingBox var8;
		for(var5 = 0; var5 < this.boundingBox.getXSize(); var5 += 4) {
			var5 += var3.nextInt(this.boundingBox.getXSize());
			if(var5 + 3 > this.boundingBox.getXSize()) {
				break;
			}

			var7 = StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var3.nextInt(var6) + 1, this.boundingBox.minZ - 1, 2, var4);
			if(var7 != null) {
				var8 = var7.getBoundingBox();
				this.chidStructures.add(new StructureBoundingBox(var8.minX, var8.minY, this.boundingBox.minZ, var8.maxX, var8.maxY, this.boundingBox.minZ + 1));
			}
		}

		for(var5 = 0; var5 < this.boundingBox.getXSize(); var5 += 4) {
			var5 += var3.nextInt(this.boundingBox.getXSize());
			if(var5 + 3 > this.boundingBox.getXSize()) {
				break;
			}

			var7 = StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX + var5, this.boundingBox.minY + var3.nextInt(var6) + 1, this.boundingBox.maxZ + 1, 0, var4);
			if(var7 != null) {
				var8 = var7.getBoundingBox();
				this.chidStructures.add(new StructureBoundingBox(var8.minX, var8.minY, this.boundingBox.maxZ - 1, var8.maxX, var8.maxY, this.boundingBox.maxZ));
			}
		}

		for(var5 = 0; var5 < this.boundingBox.getZSize(); var5 += 4) {
			var5 += var3.nextInt(this.boundingBox.getZSize());
			if(var5 + 3 > this.boundingBox.getZSize()) {
				break;
			}

			var7 = StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.minX - 1, this.boundingBox.minY + var3.nextInt(var6) + 1, this.boundingBox.minZ + var5, 1, var4);
			if(var7 != null) {
				var8 = var7.getBoundingBox();
				this.chidStructures.add(new StructureBoundingBox(this.boundingBox.minX, var8.minY, var8.minZ, this.boundingBox.minX + 1, var8.maxY, var8.maxZ));
			}
		}

		for(var5 = 0; var5 < this.boundingBox.getZSize(); var5 += 4) {
			var5 += var3.nextInt(this.boundingBox.getZSize());
			if(var5 + 3 > this.boundingBox.getZSize()) {
				break;
			}

			var7 = StructureMineshaftPieces.getNextComponent(var1, var2, var3, this.boundingBox.maxX + 1, this.boundingBox.minY + var3.nextInt(var6) + 1, this.boundingBox.minZ + var5, 3, var4);
			if(var7 != null) {
				var8 = var7.getBoundingBox();
				this.chidStructures.add(new StructureBoundingBox(this.boundingBox.maxX - 1, var8.minY, var8.minZ, this.boundingBox.maxX, var8.maxY, var8.maxZ));
			}
		}

	}

	public boolean addComponentParts(World var1, Random var2, StructureBoundingBox var3) {
		if(this.isLiquidInStructureBoundingBox(var1, var3)) {
			return false;
		} else {
			this.fillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.minY, this.boundingBox.minZ, this.boundingBox.maxX, this.boundingBox.minY, this.boundingBox.maxZ, Block.dirt.blockID, 0, true);
			this.fillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.minY + 1, this.boundingBox.minZ, this.boundingBox.maxX, Math.min(this.boundingBox.minY + 3, this.boundingBox.maxY), this.boundingBox.maxZ, 0, 0, false);
			Iterator var4 = this.chidStructures.iterator();

			while(var4.hasNext()) {
				StructureBoundingBox var5 = (StructureBoundingBox)var4.next();
				this.fillWithBlocks(var1, var3, var5.minX, var5.maxY - 2, var5.minZ, var5.maxX, var5.maxY, var5.maxZ, 0, 0, false);
			}

			this.randomlyRareFillWithBlocks(var1, var3, this.boundingBox.minX, this.boundingBox.minY + 4, this.boundingBox.minZ, this.boundingBox.maxX, this.boundingBox.maxY, this.boundingBox.maxZ, 0, false);
			return true;
		}
	}
}
