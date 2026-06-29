package net.minecraft.src;

public class BlockWood extends Block {
	public BlockWood(int var1) {
		super(var1, 4, Material.wood);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		switch(var2) {
		case 1:
			return 198;
		case 2:
			return 214;
		case 3:
			return 199;
		default:
			return 4;
		}
	}

	protected int damageDropped(int var1) {
		return var1;
	}
}
