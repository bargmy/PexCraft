package net.minecraft.src;

public class BlockSandStone extends Block {
	public BlockSandStone(int var1) {
		super(var1, 192, Material.rock);
	}

	public int getBlockTextureFromSideAndMetadata(int var1, int var2) {
		return var1 != 1 && (var1 != 0 || var2 != 1 && var2 != 2) ? (var1 == 0 ? 208 : (var2 == 1 ? 229 : (var2 == 2 ? 230 : 192))) : 176;
	}

	public int getBlockTextureFromSide(int var1) {
		return var1 == 1 ? this.blockIndexInTexture - 16 : (var1 == 0 ? this.blockIndexInTexture + 16 : this.blockIndexInTexture);
	}

	protected int damageDropped(int var1) {
		return var1;
	}
}
