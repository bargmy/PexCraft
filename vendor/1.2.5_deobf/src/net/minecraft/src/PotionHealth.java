package net.minecraft.src;

public class PotionHealth extends Potion {
	public PotionHealth(int var1, boolean var2, int var3) {
		super(var1, var2, var3);
	}

	public boolean isInstant() {
		return true;
	}

	public boolean isReady(int var1, int var2) {
		return var1 >= 1;
	}
}
