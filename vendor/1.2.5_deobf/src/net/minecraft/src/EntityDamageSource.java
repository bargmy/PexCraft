package net.minecraft.src;

public class EntityDamageSource extends DamageSource {
	protected Entity damageSourceEntity;

	public EntityDamageSource(String var1, Entity var2) {
		super(var1);
		this.damageSourceEntity = var2;
	}

	public Entity getEntity() {
		return this.damageSourceEntity;
	}
}
