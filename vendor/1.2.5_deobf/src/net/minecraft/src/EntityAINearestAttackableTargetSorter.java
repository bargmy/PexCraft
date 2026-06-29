package net.minecraft.src;

import java.util.Comparator;

public class EntityAINearestAttackableTargetSorter implements Comparator {
	private Entity theEntity;
	final EntityAINearestAttackableTarget parent;

	public EntityAINearestAttackableTargetSorter(EntityAINearestAttackableTarget var1, Entity var2) {
		this.parent = var1;
		this.theEntity = var2;
	}

	public int func_48469_a(Entity var1, Entity var2) {
		double var3 = this.theEntity.getDistanceSqToEntity(var1);
		double var5 = this.theEntity.getDistanceSqToEntity(var2);
		return var3 < var5 ? -1 : (var3 > var5 ? 1 : 0);
	}

	public int compare(Object var1, Object var2) {
		return this.func_48469_a((Entity)var1, (Entity)var2);
	}
}
