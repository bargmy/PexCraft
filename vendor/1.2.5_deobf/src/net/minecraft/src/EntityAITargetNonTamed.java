package net.minecraft.src;

public class EntityAITargetNonTamed extends EntityAINearestAttackableTarget {
	private EntityTameable field_48390_g;

	public EntityAITargetNonTamed(EntityTameable var1, Class var2, float var3, int var4, boolean var5) {
		super(var1, var2, var3, var4, var5);
		this.field_48390_g = var1;
	}

	public boolean shouldExecute() {
		return this.field_48390_g.isTamed() ? false : super.shouldExecute();
	}
}
