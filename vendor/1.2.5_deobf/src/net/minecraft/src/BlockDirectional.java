package net.minecraft.src;

public abstract class BlockDirectional extends Block {
	protected BlockDirectional(int var1, int var2, Material var3) {
		super(var1, var2, var3);
	}

	protected BlockDirectional(int var1, Material var2) {
		super(var1, var2);
	}

	public static int getDirection(int var0) {
		return var0 & 3;
	}
}
