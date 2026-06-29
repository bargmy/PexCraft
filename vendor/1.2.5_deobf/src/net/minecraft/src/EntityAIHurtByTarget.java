package net.minecraft.src;

import java.util.Iterator;
import java.util.List;

public class EntityAIHurtByTarget extends EntityAITarget {
	boolean field_48395_a;

	public EntityAIHurtByTarget(EntityLiving var1, boolean var2) {
		super(var1, 16.0F, false);
		this.field_48395_a = var2;
		this.setMutexBits(1);
	}

	public boolean shouldExecute() {
		return this.func_48376_a(this.taskOwner.getAITarget(), true);
	}

	public void startExecuting() {
		this.taskOwner.setAttackTarget(this.taskOwner.getAITarget());
		if(this.field_48395_a) {
			List var1 = this.taskOwner.worldObj.getEntitiesWithinAABB(this.taskOwner.getClass(), AxisAlignedBB.getBoundingBoxFromPool(this.taskOwner.posX, this.taskOwner.posY, this.taskOwner.posZ, this.taskOwner.posX + 1.0D, this.taskOwner.posY + 1.0D, this.taskOwner.posZ + 1.0D).expand((double)this.field_48379_d, 4.0D, (double)this.field_48379_d));
			Iterator var2 = var1.iterator();

			while(var2.hasNext()) {
				Entity var3 = (Entity)var2.next();
				EntityLiving var4 = (EntityLiving)var3;
				if(this.taskOwner != var4 && var4.getAttackTarget() == null) {
					var4.setAttackTarget(this.taskOwner.getAITarget());
				}
			}
		}

		super.startExecuting();
	}
}
