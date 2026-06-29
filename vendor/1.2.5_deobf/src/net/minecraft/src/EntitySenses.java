package net.minecraft.src;

import java.util.ArrayList;

public class EntitySenses {
	EntityLiving entityObj;
	ArrayList canSeeCachePositive = new ArrayList();
	ArrayList canSeeCacheNegative = new ArrayList();

	public EntitySenses(EntityLiving var1) {
		this.entityObj = var1;
	}

	public void clearSensingCache() {
		this.canSeeCachePositive.clear();
		this.canSeeCacheNegative.clear();
	}

	public boolean canSee(Entity var1) {
		if(this.canSeeCachePositive.contains(var1)) {
			return true;
		} else if(this.canSeeCacheNegative.contains(var1)) {
			return false;
		} else {
			Profiler.startSection("canSee");
			boolean var2 = this.entityObj.canEntityBeSeen(var1);
			Profiler.endSection();
			if(var2) {
				this.canSeeCachePositive.add(var1);
			} else {
				this.canSeeCacheNegative.add(var1);
			}

			return var2;
		}
	}
}
