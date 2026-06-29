package net.minecraft.src;

public class EntityDragonBase extends EntityLiving {
	protected int maxHealth = 100;

	public EntityDragonBase(World var1) {
		super(var1);
	}

	public int getMaxHealth() {
		return this.maxHealth;
	}

	public boolean attackEntityFromPart(EntityDragonPart var1, DamageSource var2, int var3) {
		return this.attackEntityFrom(var2, var3);
	}

	public boolean attackEntityFrom(DamageSource var1, int var2) {
		return false;
	}

	protected boolean superAttackFrom(DamageSource var1, int var2) {
		return super.attackEntityFrom(var1, var2);
	}
}
