package net.minecraft.src;

public class BlockSponge extends Block {
	protected BlockSponge(int var1) {
		super(var1, Material.sponge);
		this.blockIndexInTexture = 48;
	}

	public void onBlockAdded(World var1, int var2, int var3, int var4) {
	}

	public void onBlockRemoval(World var1, int var2, int var3, int var4) {
	}
}
