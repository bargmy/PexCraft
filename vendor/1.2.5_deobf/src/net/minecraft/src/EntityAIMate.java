package net.minecraft.src;

import java.util.Iterator;
import java.util.List;
import java.util.Random;

public class EntityAIMate extends EntityAIBase {
	private EntityAnimal theAnimal;
	World theWorld;
	private EntityAnimal targetMate;
	int field_48261_b = 0;
	float field_48262_c;

	public EntityAIMate(EntityAnimal var1, float var2) {
		this.theAnimal = var1;
		this.theWorld = var1.worldObj;
		this.field_48262_c = var2;
		this.setMutexBits(3);
	}

	public boolean shouldExecute() {
		if(!this.theAnimal.isInLove()) {
			return false;
		} else {
			this.targetMate = this.func_48258_h();
			return this.targetMate != null;
		}
	}

	public boolean continueExecuting() {
		return this.targetMate.isEntityAlive() && this.targetMate.isInLove() && this.field_48261_b < 60;
	}

	public void resetTask() {
		this.targetMate = null;
		this.field_48261_b = 0;
	}

	public void updateTask() {
		this.theAnimal.getLookHelper().setLookPositionWithEntity(this.targetMate, 10.0F, (float)this.theAnimal.getVerticalFaceSpeed());
		this.theAnimal.getNavigator().func_48667_a(this.targetMate, this.field_48262_c);
		++this.field_48261_b;
		if(this.field_48261_b == 60) {
			this.func_48257_i();
		}

	}

	private EntityAnimal func_48258_h() {
		float var1 = 8.0F;
		List var2 = this.theWorld.getEntitiesWithinAABB(this.theAnimal.getClass(), this.theAnimal.boundingBox.expand((double)var1, (double)var1, (double)var1));
		Iterator var3 = var2.iterator();

		EntityAnimal var5;
		do {
			if(!var3.hasNext()) {
				return null;
			}

			Entity var4 = (Entity)var3.next();
			var5 = (EntityAnimal)var4;
		} while(!this.theAnimal.func_48135_b(var5));

		return var5;
	}

	private void func_48257_i() {
		EntityAnimal var1 = this.theAnimal.spawnBabyAnimal(this.targetMate);
		if(var1 != null) {
			this.theAnimal.setGrowingAge(6000);
			this.targetMate.setGrowingAge(6000);
			this.theAnimal.resetInLove();
			this.targetMate.resetInLove();
			var1.setGrowingAge(-24000);
			var1.setLocationAndAngles(this.theAnimal.posX, this.theAnimal.posY, this.theAnimal.posZ, 0.0F, 0.0F);
			this.theWorld.spawnEntityInWorld(var1);
			Random var2 = this.theAnimal.getRNG();

			for(int var3 = 0; var3 < 7; ++var3) {
				double var4 = var2.nextGaussian() * 0.02D;
				double var6 = var2.nextGaussian() * 0.02D;
				double var8 = var2.nextGaussian() * 0.02D;
				this.theWorld.spawnParticle("heart", this.theAnimal.posX + (double)(var2.nextFloat() * this.theAnimal.width * 2.0F) - (double)this.theAnimal.width, this.theAnimal.posY + 0.5D + (double)(var2.nextFloat() * this.theAnimal.height), this.theAnimal.posZ + (double)(var2.nextFloat() * this.theAnimal.width * 2.0F) - (double)this.theAnimal.width, var4, var6, var8);
			}

		}
	}
}
