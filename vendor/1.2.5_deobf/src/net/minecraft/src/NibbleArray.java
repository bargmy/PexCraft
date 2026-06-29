package net.minecraft.src;

public class NibbleArray {
	public final byte[] data;
	private final int depthBits;
	private final int depthBitsPlusFour;

	public NibbleArray(int var1, int var2) {
		this.data = new byte[var1 >> 1];
		this.depthBits = var2;
		this.depthBitsPlusFour = var2 + 4;
	}

	public NibbleArray(byte[] var1, int var2) {
		this.data = var1;
		this.depthBits = var2;
		this.depthBitsPlusFour = var2 + 4;
	}

	public int get(int var1, int var2, int var3) {
		int var4 = var2 << this.depthBitsPlusFour | var3 << this.depthBits | var1;
		int var5 = var4 >> 1;
		int var6 = var4 & 1;
		return var6 == 0 ? this.data[var5] & 15 : this.data[var5] >> 4 & 15;
	}

	public void set(int var1, int var2, int var3, int var4) {
		int var5 = var2 << this.depthBitsPlusFour | var3 << this.depthBits | var1;
		int var6 = var5 >> 1;
		int var7 = var5 & 1;
		if(var7 == 0) {
			this.data[var6] = (byte)(this.data[var6] & 240 | var4 & 15);
		} else {
			this.data[var6] = (byte)(this.data[var6] & 15 | (var4 & 15) << 4);
		}

	}
}
