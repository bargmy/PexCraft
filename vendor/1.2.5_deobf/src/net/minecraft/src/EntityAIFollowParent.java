package net.minecraft.src;

import java.util.Iterator;
import java.util.List;

public class EntityAIFollowParent extends EntityAIBase {
	EntityAnimal childAnimal;
	EntityAnimal parentAnimal;
	float field_48248_c;
	private int field_48246_d;

	public EntityAIFollowParent(EntityAnimal var1, float var2) {
		this.childAnimal = var1;
		this.field_48248_c = var2;
	}

	public boolean shouldExecute() {
		if(this.childAnimal.getGrowingAge() >= 0) {
			return false;
		} else {
			List var1 = this.childAnimal.worldObj.getEntitiesWithinAABB(this.childAnimal.getClass(), this.childAnimal.boundingBox.expand(8.0D, 4.0D, 8.0D));
			EntityAnimal var2 = null;
			double var3 = Double.MAX_VALUE;
			Iterator var5 = var1.iterator();

			while(var5.hasNext()) {
				Entity var6 = (Entity)var5.next();
				EntityAnimal var7 = (EntityAnimal)var6;
				if(var7.getGrowingAge() >= 0) {
					double var8 = this.childAnimal.getDistanceSqToEntity(var7);
					if(var8 <= var3) {
						var3 = var8;
						var2 = var7;
					}
				}
			}

			if(var2 == null) {
				return false;
			} else if(var3 < 9.0D) {
				return false;
			} else {
				this.parentAnimal = var2;
				return true;
			}
		}
	}

	public boolean continueExecuting() {
		if(!this.parentAnimal.isEntityAlive()) {
			return false;
		} else {
			double var1 = this.childAnimal.getDistanceSqToEntity(this.parentAnimal);
			return var1 >= 9.0D && var1 <= 256.0D;
		}
	}

	public void startExecuting() {
		this.field_48246_d = 0;
	}

	public void resetTask() {
		this.parentAnimal = null;
	}

	public void updateTask() {
		if(--this.field_48246_d <= 0) {
			this.field_48246_d = 10;
			this.childAnimal.getNavigator().func_48667_a(this.parentAnimal, this.field_48248_c);
		}
	}
}
