package net.minecraft.src;

import java.util.Iterator;
import java.util.List;

public class EntityAIPlay extends EntityAIBase {
	private EntityVillager villagerObj;
	private EntityLiving targetVillager;
	private float field_48358_c;
	private int field_48356_d;

	public EntityAIPlay(EntityVillager var1, float var2) {
		this.villagerObj = var1;
		this.field_48358_c = var2;
		this.setMutexBits(1);
	}

	public boolean shouldExecute() {
		if(this.villagerObj.getGrowingAge() >= 0) {
			return false;
		} else if(this.villagerObj.getRNG().nextInt(400) != 0) {
			return false;
		} else {
			List var1 = this.villagerObj.worldObj.getEntitiesWithinAABB(EntityVillager.class, this.villagerObj.boundingBox.expand(6.0D, 3.0D, 6.0D));
			double var2 = Double.MAX_VALUE;
			Iterator var4 = var1.iterator();

			while(var4.hasNext()) {
				Entity var5 = (Entity)var4.next();
				if(var5 != this.villagerObj) {
					EntityVillager var6 = (EntityVillager)var5;
					if(!var6.getIsPlayingFlag() && var6.getGrowingAge() < 0) {
						double var7 = var6.getDistanceSqToEntity(this.villagerObj);
						if(var7 <= var2) {
							var2 = var7;
							this.targetVillager = var6;
						}
					}
				}
			}

			if(this.targetVillager == null) {
				Vec3D var9 = RandomPositionGenerator.func_48622_a(this.villagerObj, 16, 3);
				if(var9 == null) {
					return false;
				}
			}

			return true;
		}
	}

	public boolean continueExecuting() {
		return this.field_48356_d > 0;
	}

	public void startExecuting() {
		if(this.targetVillager != null) {
			this.villagerObj.setIsPlayingFlag(true);
		}

		this.field_48356_d = 1000;
	}

	public void resetTask() {
		this.villagerObj.setIsPlayingFlag(false);
		this.targetVillager = null;
	}

	public void updateTask() {
		--this.field_48356_d;
		if(this.targetVillager != null) {
			if(this.villagerObj.getDistanceSqToEntity(this.targetVillager) > 4.0D) {
				this.villagerObj.getNavigator().func_48667_a(this.targetVillager, this.field_48358_c);
			}
		} else if(this.villagerObj.getNavigator().noPath()) {
			Vec3D var1 = RandomPositionGenerator.func_48622_a(this.villagerObj, 16, 3);
			if(var1 == null) {
				return;
			}

			this.villagerObj.getNavigator().func_48666_a(var1.xCoord, var1.yCoord, var1.zCoord, this.field_48358_c);
		}

	}
}
