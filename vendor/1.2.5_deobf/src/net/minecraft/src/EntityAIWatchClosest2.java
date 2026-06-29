package net.minecraft.src;

public class EntityAIWatchClosest2 extends EntityAIWatchClosest {
	public EntityAIWatchClosest2(EntityLiving var1, Class var2, float var3) {
		super(var1, var2, var3);
		this.setMutexBits(3);
	}

	public EntityAIWatchClosest2(EntityLiving var1, Class var2, float var3, float var4) {
		super(var1, var2, var3, var4);
		this.setMutexBits(3);
	}
}
