package net.minecraft.src;

class LongHashMapEntry {
	final long key;
	Object value;
	LongHashMapEntry nextEntry;
	final int hash;

	LongHashMapEntry(int var1, long var2, Object var4, LongHashMapEntry var5) {
		this.value = var4;
		this.nextEntry = var5;
		this.key = var2;
		this.hash = var1;
	}

	public final long getKey() {
		return this.key;
	}

	public final Object getValue() {
		return this.value;
	}

	public final boolean equals(Object var1) {
		if(!(var1 instanceof LongHashMapEntry)) {
			return false;
		} else {
			LongHashMapEntry var2 = (LongHashMapEntry)var1;
			Long var3 = Long.valueOf(this.getKey());
			Long var4 = Long.valueOf(var2.getKey());
			if(var3 == var4 || var3 != null && var3.equals(var4)) {
				Object var5 = this.getValue();
				Object var6 = var2.getValue();
				if(var5 == var6 || var5 != null && var5.equals(var6)) {
					return true;
				}
			}

			return false;
		}
	}

	public final int hashCode() {
		return LongHashMap.getHashCode(this.key);
	}

	public final String toString() {
		return this.getKey() + "=" + this.getValue();
	}
}
