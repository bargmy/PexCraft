package net.minecraft.src;

public class SaveFormatComparator implements Comparable {
	private final String fileName;
	private final String displayName;
	private final long lastTimePlayed;
	private final long sizeOnDisk;
	private final boolean requiresConversion;
	private final int gameType;
	private final boolean hardcore;

	public SaveFormatComparator(String var1, String var2, long var3, long var5, int var7, boolean var8, boolean var9) {
		this.fileName = var1;
		this.displayName = var2;
		this.lastTimePlayed = var3;
		this.sizeOnDisk = var5;
		this.gameType = var7;
		this.requiresConversion = var8;
		this.hardcore = var9;
	}

	public String getFileName() {
		return this.fileName;
	}

	public String getDisplayName() {
		return this.displayName;
	}

	public boolean requiresConversion() {
		return this.requiresConversion;
	}

	public long getLastTimePlayed() {
		return this.lastTimePlayed;
	}

	public int compareTo(SaveFormatComparator var1) {
		return this.lastTimePlayed < var1.lastTimePlayed ? 1 : (this.lastTimePlayed > var1.lastTimePlayed ? -1 : this.fileName.compareTo(var1.fileName));
	}

	public int getGameType() {
		return this.gameType;
	}

	public boolean isHardcoreModeEnabled() {
		return this.hardcore;
	}

	public int compareTo(Object var1) {
		return this.compareTo((SaveFormatComparator)var1);
	}
}
