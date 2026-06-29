package net.minecraft.src;

public final class ProfilerResult implements Comparable {
	public double sectionPercentage;
	public double globalPercentage;
	public String name;

	public ProfilerResult(String var1, double var2, double var4) {
		this.name = var1;
		this.sectionPercentage = var2;
		this.globalPercentage = var4;
	}

	public int compareProfilerResult(ProfilerResult var1) {
		return var1.sectionPercentage < this.sectionPercentage ? -1 : (var1.sectionPercentage > this.sectionPercentage ? 1 : var1.name.compareTo(this.name));
	}

	public int getDisplayColor() {
		return (this.name.hashCode() & 11184810) + 4473924;
	}

	public int compareTo(Object var1) {
		return this.compareProfilerResult((ProfilerResult)var1);
	}
}
