package net.minecraft.src;

public class PotionEffect {
	private int potionID;
	private int duration;
	private int amplifier;

	public PotionEffect(int var1, int var2, int var3) {
		this.potionID = var1;
		this.duration = var2;
		this.amplifier = var3;
	}

	public PotionEffect(PotionEffect var1) {
		this.potionID = var1.potionID;
		this.duration = var1.duration;
		this.amplifier = var1.amplifier;
	}

	public void combine(PotionEffect var1) {
		if(this.potionID != var1.potionID) {
			System.err.println("This method should only be called for matching effects!");
		}

		if(var1.amplifier > this.amplifier) {
			this.amplifier = var1.amplifier;
			this.duration = var1.duration;
		} else if(var1.amplifier == this.amplifier && this.duration < var1.duration) {
			this.duration = var1.duration;
		}

	}

	public int getPotionID() {
		return this.potionID;
	}

	public int getDuration() {
		return this.duration;
	}

	public int getAmplifier() {
		return this.amplifier;
	}

	public boolean onUpdate(EntityLiving var1) {
		if(this.duration > 0) {
			if(Potion.potionTypes[this.potionID].isReady(this.duration, this.amplifier)) {
				this.performEffect(var1);
			}

			this.deincrementDuration();
		}

		return this.duration > 0;
	}

	private int deincrementDuration() {
		return --this.duration;
	}

	public void performEffect(EntityLiving var1) {
		if(this.duration > 0) {
			Potion.potionTypes[this.potionID].performEffect(var1, this.amplifier);
		}

	}

	public String getEffectName() {
		return Potion.potionTypes[this.potionID].getName();
	}

	public int hashCode() {
		return this.potionID;
	}

	public String toString() {
		String var1 = "";
		if(this.getAmplifier() > 0) {
			var1 = this.getEffectName() + " x " + (this.getAmplifier() + 1) + ", Duration: " + this.getDuration();
		} else {
			var1 = this.getEffectName() + ", Duration: " + this.getDuration();
		}

		return Potion.potionTypes[this.potionID].isUsable() ? "(" + var1 + ")" : var1;
	}

	public boolean equals(Object var1) {
		if(!(var1 instanceof PotionEffect)) {
			return false;
		} else {
			PotionEffect var2 = (PotionEffect)var1;
			return this.potionID == var2.potionID && this.amplifier == var2.amplifier && this.duration == var2.duration;
		}
	}
}
