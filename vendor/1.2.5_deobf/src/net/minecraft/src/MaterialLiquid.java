package net.minecraft.src;

public class MaterialLiquid extends Material {
	public MaterialLiquid(MapColor var1) {
		super(var1);
		this.setGroundCover();
		this.setNoPushMobility();
	}

	public boolean isLiquid() {
		return true;
	}

	public boolean blocksMovement() {
		return false;
	}

	public boolean isSolid() {
		return false;
	}
}
